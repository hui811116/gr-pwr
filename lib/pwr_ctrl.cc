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
  	enum PWR_CTRL_LEVEL{
  		LOWEST=0x11,
  		LV01=0x22,
  		LV02=0x33,
  		LV03=0x44,
  		MEDIUM=0x55,
  		LV11=0x66,
  		LV12=0x77,
  		LV13=0x88,
  		HIGH=0x99,
  		LV21=0xAA,
  		LV22=0xBB,
  		LV23=0xCC
  		// residual levels for construction
  	};
    class pwr_ctrl_impl : public pwr_ctrl
    {
    public:
    	pwr_ctrl_impl() : block("pwr_ctrl",
    		gr::io_signature::make(0,0,0),
    		gr::io_signature::make(0,0,0)),
    		d_out_port(pmt::mp("msg_out")),
    		d_pwr_out(pmt::mp("pwr_out")),
    		d_pwr_in(pmt::mp("pwr_in")),
    		d_current_pwr_level(MEDIUM)
    	{
    		message_port_register_in(d_pwr_in);
    		message_port_register_out(d_out_port);
    		message_port_register_out(d_pwr_out);
    		set_msg_handler(d_pwr_in,boost::bind(&pwr_ctrl_impl::pwr_in,this,_1));
    	}
    	~pwr_ctrl_impl(){}
    	

    private:
    	
    	void pwr_in(pmt::pmt_t msg)
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
    			if(new_level != d_current_pwr_level){
    				// should change power
    				// change power here
    				d_current_pwr_level = new_level;
    				d_pwr_buf[0] = new_level;
    				pmt::pmt_t pwr_quest = pmt::make_blob(d_pwr_buf,1);
    				message_port_pub(d_pwr_out,pmt::cons(pmt::PMT_NIL,pwr_quest));
    			}
    		}else{
    			// useless
    		}
    	}
    	
    	int get_pwr_level(const unsigned char pwr_byte)
    	{
    		// TODO
    		// Once specifies the pwr levels, extract value from byte
    			switch(pwr_byte)
    			{
    				case LOWEST:
    				case LV01:
    				case LV02:
    				case LV03:
    				case MEDIUM:
    				case LV11:
    				case LV12:
    				case LV13:
    				case HIGH:
    				case LV21:
    				case LV22:
    				case LV23:
    					return pwr_byte;
    				break;
    				default:
    					return d_current_pwr_level;
    				break;
    			}
    	}
    	const pmt::pmt_t d_out_port;
    	const pmt::pmt_t d_pwr_out;
    	const pmt::pmt_t d_pwr_in;
    	gr::thread::mutex d_mutex;
    	unsigned char d_buf[256];
    	unsigned char d_pwr_buf[2];
    	int d_current_pwr_level;

    };

    pwr_ctrl::sptr
    pwr_ctrl::make()
    {
    	return gnuradio::get_initial_sptr(new pwr_ctrl_impl());
    }
  } /* namespace pwr */
} /* namespace gr */

