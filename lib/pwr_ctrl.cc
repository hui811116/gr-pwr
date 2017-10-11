/* -*- c++ -*- */
/* 
 * Copyright 2017 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include <pwr/pwr_ctrl.h>
#include <gnuradio/block_detail.h>
#include <cstring>

namespace gr {
  namespace pwr {
    #define UPDATE_PERIOD_MS 1000
    #define DEFAULT_PWR_LEVEL 4
  	
    static unsigned char d_pwr_map[12] = {
        0x11,0x22,0x33,0x44,
        0x55,0x66,0x77,0x88,
        0x99,0xAA,0xBB,0XCC
    };
    static double d_gain_map[12] = {
        0.100,0.173,0.246,0.318,
        0.391,0.464,0.536,0.609,
        0.682,0.755,0.827,0.9
    };
    static int d_gain_len = 12;
    class pwr_ctrl_impl : public pwr_ctrl
    {
    public:
    	pwr_ctrl_impl(int target_val) : block("pwr_ctrl",
    		gr::io_signature::make(0,0,0),
    		gr::io_signature::make(0,0,0)),
    		d_out_port(pmt::mp("msg_out")),
    		d_pwr_out(pmt::mp("pwr_out")),
    		d_pwr_in(pmt::mp("pwr_in"))
    	{
    		
    		message_port_register_out(d_out_port);
    		message_port_register_out(d_pwr_out);
            message_port_register_in(d_pwr_in);
    		set_msg_handler(d_pwr_in,boost::bind(&pwr_ctrl_impl::pwrmsg_in,this,_1));
            // -1 for initial state
            set_target(target_val);
    	}
    	~pwr_ctrl_impl(){}
        enum PWR_CTRL_LEVEL{
            LOWEST=0x11,LV01=0x22,LV02=0x33,LV03=0x44,
            MEDIUM=0x55,LV11=0x66,LV12=0x77,LV13=0x88,
            HIGH=0x99,LV21=0xAA,LV22=0xBB,LV23=0xCC,
            INIT=0x00
            // residual levels for construction
        };
        
        bool start()
        {
            d_finished = false;
            d_thread = boost::shared_ptr<gr::thread::thread>
                (new gr::thread::thread(boost::bind(&pwr_ctrl_impl::run,this)));
            return block::start();
        }
        bool stop()
        {
            d_finished = true;
            d_change_pwr.notify_one();
            d_thread->interrupt();
            d_thread->join();
            return block::stop();
        }
    	void pwrmsg_in(pmt::pmt_t msg)
        {
            gr::thread::scoped_lock guard(d_mutex);
            pmt::pmt_t k = pmt::car(msg);
            pmt::pmt_t v = pmt::cdr(msg);
            assert(pmt::is_blob(v));
            size_t io(0);
            const uint8_t* uvec = pmt::u8vector_elements(v,io);
            if(io>1){
                memcpy(d_buf,uvec,sizeof(char) * (io-1));
                pmt::pmt_t blob = pmt::make_blob(d_buf,io-1);
                message_port_pub(d_out_port,pmt::cons(pmt::PMT_NIL,blob));
            }
            if(io>0){
                uint8_t pwr_byte = uvec[0];
                int new_level = get_pwr_level(pwr_byte);
                if(new_level < 0){
                    return;
                }
                if(d_current_pwr_level<0){
                    d_current_pwr_level = DEFAULT_PWR_LEVEL;
                }
                if(d_pwr_map[new_level] != d_recent_pwr_level){
                    // should change power
                    d_recent_pwr_level = d_pwr_map[new_level];
                    switch(d_target_level){
                        case LV02:
                            if(new_level>3){
                                d_current_pwr_level--;
                                if(d_current_pwr_level<0){
                                    d_current_pwr_level =0;
                                }
                                d_change_pwr.notify_one();
                            }else{
                                // matched level
                            }
                        break;
                        case LV12:
                            if(new_level<4){
                                d_current_pwr_level++;
                                if(d_current_pwr_level==d_gain_len){
                                    d_current_pwr_level = d_gain_len -1;
                                }
                                d_change_pwr.notify_one();
                            }else if(new_level>7){
                                d_current_pwr_level--;
                                if(d_current_pwr_level<0){
                                    d_current_pwr_level =0;
                                }
                                d_change_pwr.notify_one();
                            }else{
                                // matched level
                            }
                        break;
                        case LV22:
                            if(new_level<8){
                                d_current_pwr_level++;
                                if(d_current_pwr_level==d_gain_len){
                                    d_current_pwr_level = d_gain_len -1;
                                }
                                d_change_pwr.notify_one();
                            }else{
                                // matched level
                            }
                        break;
                        default:
                            throw std::runtime_error("invalid power target");
                        break;
                    }
                }
            }else{
                // useless
            }
        }
        void set_target(int target)
        {
            gr::thread::scoped_lock guard(d_mutex);
            // -1 for initialization
            switch(target){
                case 0:
                    d_target_level = LV02;
                    d_current_pwr_level = -1;
                    break;
                case 1:
                    d_target_level = LV12;
                    d_current_pwr_level = -1;
                    break;
                case 2:
                    d_target_level = LV22;
                    d_current_pwr_level = -1;
                    break;
                default:
                    d_target_level = LV12;
                    d_current_pwr_level = -1;
                    break;
            }
        }
        int target() const
        {
            int return_level;
            switch(d_target_level){
                case LV02:
                    return_level =0;
                    break;
                case LV12:
                    return_level = 1;
                    break;
                case LV22:
                    return_level = 2;
                    break;
                default:
                    throw std::runtime_error("undefined target");
                    break;
            }
            return return_level;
        }
    private:
        void run()
        {
            pmt::pmt_t value;
            while(true){
                if(d_current_pwr_level<0){
                    // in initial state
                    value = pmt::from_double(d_gain_map[DEFAULT_PWR_LEVEL]);
                    message_port_pub(d_pwr_out,pmt::cons(pmt::PMT_NIL,value));
                    gr::thread::scoped_lock lock(d_mutex);
                    d_change_pwr.timed_wait(lock,boost::posix_time::milliseconds(UPDATE_PERIOD_MS));
                    lock.unlock();
                }else{
                    value = pmt::from_double(d_gain_map[d_current_pwr_level]);
                    message_port_pub(d_pwr_out,pmt::cons(pmt::PMT_NIL,value));
                    gr::thread::scoped_lock lock(d_mutex);
                    d_change_pwr.wait(lock);
                    lock.unlock();
                }
                if(d_finished){
                    return;
                }
            }
        }
    	int get_pwr_level(const unsigned char pwr_byte)
    	{
    		// Once specifies the pwr levels, extract value from byte
    			switch(pwr_byte)
    			{
    				case LOWEST:
                        return 0;
                        break;
    				case LV01:
                        return 1;
                        break;
    				case LV02:
                        return 2;
                        break;
    				case LV03:
                        return 3;
                        break;
    				case MEDIUM:
                        return 4;
                        break;
    				case LV11:
                        return 5;
                        break;
    				case LV12:
                        return 6;
                        break;
    				case LV13:
                        return 7;
                        break;
    				case HIGH:
                        return 8;
                        break;
    				case LV21:
                        return 9;
                        break;
    				case LV22:
                        return 10;
                        break;
    				case LV23:
                        return 11;
                        break;
                    case INIT:
                        return 12;
                        break;
    				default:
    					return -1;
    				    break;
    			}
    	}
    	const pmt::pmt_t d_out_port;
    	const pmt::pmt_t d_pwr_out;
    	const pmt::pmt_t d_pwr_in;
    	gr::thread::mutex d_mutex;
        boost::shared_ptr<gr::thread::thread> d_thread;
        gr::thread::condition_variable d_change_pwr;
        bool d_finished;
    	unsigned char d_buf[256];
    	unsigned char d_pwr_buf[2];
    	int d_current_pwr_level;
        int d_recent_pwr_level;
        int d_target_level;
    };

    pwr_ctrl::sptr
    pwr_ctrl::make(int target_val)
    {
    	return gnuradio::get_initial_sptr(new pwr_ctrl_impl(target_val));
    }
  } /* namespace pwr */
} /* namespace gr */

