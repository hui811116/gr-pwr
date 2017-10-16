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
#include <pwr/pwr_sender.h>
#include <gnuradio/block_detail.h>
#include <cstring>
#include <fstream>

namespace gr {
  namespace pwr {
  	#define MAXLEN 123
  	#define SEQNUM 4
    #define d_debug 0
    #define dout d_debug && std::cout
    class pwr_sender_impl : public pwr_sender
    {
    public:
    	pwr_sender_impl(const std::string& filename, float timeout) : block("pwr_sender",
    		gr::io_signature::make(0,0,0),
    		gr::io_signature::make(0,0,0)),
    		d_feb_in(pmt::mp("feb_in")),
    		d_pld_out(pmt::mp("pld_out")),
    		d_retry_lim(10)
    	{
    		message_port_register_in(d_feb_in);
    		message_port_register_out(d_pld_out);
    		set_msg_handler(d_feb_in,boost::bind(&pwr_sender_impl::feb_in,this,_1));
    		if(!read_file(filename)){
    			throw std::runtime_error("Invalid source file, please check and restart...");
    		}
    		d_seqno = 0;
            d_timeout = timeout;
    	}
    	~pwr_sender_impl()
        {
            
        }
    	bool start()
    	{
    		d_finished = false;
    		d_sys_time = boost::posix_time::microsec_clock::local_time();
    		d_thread = boost::shared_ptr<gr::thread::thread>
    			(new gr::thread::thread(boost::bind(&pwr_sender_impl::run,this)));
    		return block::start();
    	}
    	bool stop()
    	{
    		d_finished = true;
    		d_thread->interrupt();
    		d_thread->join();
    		boost::posix_time::time_duration diff = boost::posix_time::microsec_clock::local_time()-d_sys_time;
    		// diff.total_milliseconds()/1000.0;
    		return block::stop();
    	}
        void feb_in(pmt::pmt_t msg)
        {
            pmt::pmt_t k = pmt::car(msg);
            pmt::pmt_t v = pmt::cdr(msg);
            assert(pmt::is_blob(v));
            // assume crc is already done in PHY layer
            size_t io(0);
            const uint8_t* uvec = pmt::u8vector_elements(v,io);
            if(io==4){
                uint16_t base1, base2;
                base1 = uvec[0]<<8;
                base1|= uvec[1];
                base2 = uvec[2]<<8;
                base2|= uvec[3];
                if(base1==base2 && base1 == d_seqno){
                    d_acked = true;
                    d_ack_received.notify_one();
                    // pwr sends next message once previous transmission acked
                }
            }
        }
    private:
    	void run()
    	{
    		int retry = 0;
    		while(true){
    			d_acked = false;
    			generate_msg();
                dout<<"sending packet with seqno = "<<d_seqno<<std::endl;
    			message_port_pub(d_pld_out,d_cur_msg);
    			retry++;
    			gr::thread::scoped_lock lock(d_mutex);
    			d_ack_received.timed_wait(lock,boost::posix_time::milliseconds(d_timeout));
    			lock.unlock();
    			if(d_finished){
    				return;
    			}
    			if(d_acked){
    				// successfully acked
                    dout << "successfully acked a packet"<<std::endl;
    				retry =0;
    				d_seqno = (d_seqno==0xffff || d_seqno == d_data_src.size()-1) ? 0 : d_seqno+1;
    			}else if(retry == d_retry_lim){
    				// reach retry limit, sleep 10 seconds
                    dout << "Timeout and exceed retry limits, wait for 10 seconds and resume" <<std::endl;
    				boost::this_thread::sleep(boost::posix_time::seconds(10.0));
    				if(d_finished){
    					return;
    				}
    				retry =0;
    			}

    		}
    	}
    	bool read_file(const std::string& filename)
    	{
    		gr::thread::scoped_lock guard(d_mutex);
          	std::string str,line;
          	d_file.open(filename.c_str(),std::fstream::in);
          	if(!d_file.is_open()){
            	return false;
          	}
          	while(getline(d_file,line,'\n')){
            	std::istringstream temp(line);
            	std::vector<uint8_t> u8;
            	while(getline(temp,str,',')){
              	int tmp = std::atoi(str.c_str());
              	if(u8.size()>MAXLEN){
                	throw std::runtime_error("message to be transmitted exceed the maximum payload length");
              	}
              	u8.push_back((uint8_t)tmp);
            	}
            	d_data_src.push_back(u8);
          	}
          	d_file.close();
          	return true;
    	}
    	void generate_msg()
    	{
    		gr::thread::scoped_lock guard(d_mutex);
    		const uint8_t* u8_seq = (const uint8_t*) & d_seqno;
    		d_buf[0] = u8_seq[1];
    		d_buf[1] = u8_seq[0];
    		d_buf[2] = u8_seq[1];
    		d_buf[3] = u8_seq[0];
    		memcpy(d_buf+SEQNUM, d_data_src[d_seqno].data(),sizeof(char)* d_data_src[d_seqno].size());
    		pmt::pmt_t blob = pmt::make_blob(d_buf,SEQNUM+d_data_src[d_seqno].size());
    		d_cur_msg = pmt::cons(pmt::PMT_NIL,blob);
    	} 
    	const pmt::pmt_t d_feb_in;
    	const pmt::pmt_t d_pld_out;
    	pmt::pmt_t d_cur_msg;
    	boost::shared_ptr<gr::thread::thread> d_thread;
    	boost::posix_time::ptime d_sys_time;
    	gr::thread::mutex d_mutex;
    	gr::thread::condition_variable d_ack_received;
    	bool d_finished;
    	bool d_acked;
    	std::fstream d_file;
    	std::vector< std::vector<unsigned char> > d_data_src;
    	float d_timeout;
    	const int d_retry_lim;
    	uint16_t d_seqno;
    	unsigned char d_buf[256];

    };

    pwr_sender::sptr
    pwr_sender::make(const std::string& filename, float timeout)
    {
    	return gnuradio::get_initial_sptr(new pwr_sender_impl(filename,timeout));
    }

  } /* namespace pwr */
} /* namespace gr */

