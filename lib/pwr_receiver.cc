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
#include <pwr/pwr_receiver.h>
#include <gnuradio/block_detail.h>

namespace gr {
  namespace pwr {
    #define d_debug 0
    #define dout d_debug && std::cout
  	#define SEQLEN 4
    class pwr_receiver_impl : public pwr_receiver
    {
    public:
    	pwr_receiver_impl(int expect_pkt) : block("pwr_receiver",
    				gr::io_signature::make(0,0,0),
    				gr::io_signature::make(0,0,0)),
    				d_phy_in(pmt::mp("phy_in")),
    				d_ack_out(pmt::mp("ack_out")),
    				d_pld_out(pmt::mp("pld_out"))
    	{
    		message_port_register_in(d_phy_in);
    		message_port_register_out(d_ack_out);
    		message_port_register_out(d_pld_out);
    		set_msg_handler(d_phy_in,boost::bind(&pwr_receiver_impl::phy_in,this,_1));
    		set_expect_pkt(expect_pkt);
    		d_last_seq = -1; // minus for assurance
    	}
    	~pwr_receiver_impl(){}
    	void phy_in(pmt::pmt_t msg)
    	{
    		gr::thread::scoped_lock guard(d_mutex);
    		pmt::pmt_t k = pmt::car(msg);
    		pmt::pmt_t v = pmt::cdr(msg);
    		assert(pmt::is_blob(v));
    		size_t io(0);
    		uint16_t base;
    		const uint8_t* uvec = pmt::u8vector_elements(v,io);
    		if(!crc_hdr(uvec,io)){
    			return;
    		}
    		int comp_seq = (int) base;
    		if(comp_seq != d_last_seq){
    			d_last_seq = comp_seq;
    			d_pkt_cnt++;
    			if(d_pkt_cnt%d_expect_pkt==0){
    				// complete one transmission
    				boost::posix_time::time_duration diff;
    				diff = boost::posix_time::microsec_clock::local_time() - d_sys_time;
    				// this is the time to complete transmission of a file
    				// very raw and not precise since data may contain error
    				// diff.total_milliseconds();
    				// update system time
    				d_sys_time = boost::posix_time::microsec_clock::local_time();
    			}
    		}
    		// handle ack
    		memcpy(d_buf,uvec,sizeof(char)*SEQLEN);
    		pmt::pmt_t ack_out = pmt::make_blob(d_buf,SEQLEN);
    		message_port_pub(d_ack_out,pmt::cons(pmt::PMT_NIL,ack_out));
    		// handle pdu
    		base = uvec[0]<<8;
    		base|= uvec[1];
    		pmt::pmt_t pdu_out = pmt::make_blob(uvec+SEQLEN,io-SEQLEN);
    		pmt::pmt_t dict = pmt::make_dict();
    		dict = pmt::dict_add(dict,pmt::intern("seqno"),pmt::from_long(base));
    		message_port_pub(d_pld_out,pmt::cons(dict,pdu_out));
    	}
    	void set_reset(int reset)
    	{
    		gr::thread::scoped_lock guard(d_mutex);
    		if(!d_reset_request && reset){
    			// need to reset
                dout<<"<Pwr RX>Reset function is called"<<std::endl;
    			d_reset_request = true;
    			d_reset_received.notify_one();
    		}
    	}
    	int reset() const
    	{
    		return (d_reset_request!=0)? 1 : 0;
    	}
    	void set_expect_pkt(int expect_pkt)
    	{
    		gr::thread::scoped_lock gurad(d_mutex);
    		if(expect_pkt>0){
    			d_expect_pkt = expect_pkt;
    			d_pkt_cnt = 0;
    			d_sys_time = boost::posix_time::microsec_clock::local_time();
    		}else{
    			throw std::runtime_error("Invalid expect packets size");
    		}
    	}
    	int expect_pkt() const
    	{
    		return d_expect_pkt;
    	}
    	bool start()
    	{
    		d_finished = false;
    		d_thread = boost::shared_ptr<gr::thread::thread>
    			(new gr::thread::thread(&pwr_receiver_impl::run,this));
    		return block::start();
    	}
    	bool stop()
    	{
    		d_finished = true;
    		d_reset_received.notify_one();
    		d_thread->interrupt();
    		d_thread->join();
    		return block::stop();
    	}
    private:
    	void run()
    	{
    		d_reset_request = false;
    		while(true){
    			gr::thread::scoped_lock lock(d_mutex);
    			d_reset_received.wait(lock);
    			lock.unlock();
    			if(d_finished){
    				return;
    			}else if(d_reset_request){
                    dout<<"<PWR RX>In thread function, reset system time and packet counts"<<std::endl;
    				d_sys_time = boost::posix_time::microsec_clock::local_time();
    				d_pkt_cnt=0;
    				d_reset_request = false;
    			}
    		}
    	}
    	bool crc_hdr(const uint8_t* uvec, size_t io)
    	{
    		if(io<4){
    			return false;
    		}else if(uvec[0]!=uvec[2] || uvec[1]!=uvec[3]){
    			return false;
    		}
    		return true;
    	}
    	const pmt::pmt_t d_phy_in;
    	const pmt::pmt_t d_ack_out;
    	const pmt::pmt_t d_pld_out;
    	gr::thread::mutex d_mutex;
    	unsigned char d_buf[128];
    	int d_expect_pkt;
    	int d_pkt_cnt;
    	long int d_last_seq;
    	gr::thread::condition_variable d_reset_received;
    	boost::shared_ptr<gr::thread::thread> d_thread;
    	boost::posix_time::ptime d_sys_time;
    	bool d_finished;
    	bool d_reset_request;
    };

    pwr_receiver::sptr
    pwr_receiver::make(int expect_pkt)
    {
    	return gnuradio::get_initial_sptr(new pwr_receiver_impl(expect_pkt));
    }

  } /* namespace pwr */
} /* namespace gr */

