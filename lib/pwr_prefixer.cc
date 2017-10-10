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
#include <pwr/pwr_prefixer.h>
#include <gnuradio/block_detail.h>
#include <gnuradio/math.h>

namespace gr {
  namespace pwr {
  	static pmt::pmt_t d_pwr_tag = pmt::intern("pwr_tag");
  	enum PWR_LEVEL{
  		INIT = 0x00,
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
  	};
    static uint8_t d_pwr_map[13] = {
        0x11,0x22,0x33,0x44,
        0x55,0x66,0x77,0x88,
        0x99,0xAA,0xBB,0xCC,
        0x00
    };
    static int d_pwr_map_size = 12;
  	/* ******************************************************************
  	// FIXME
  	// ******************************************************************
  	// current version of power control is manually specified
  	// for automatic power level control, a initialization phase
  	// should be implemented. This requires more detailed considerations.
  	******************************************************************* */ 
    class pwr_prefixer_impl : public pwr_prefixer
    {
    public:
    	pwr_prefixer_impl(float lowest_db,float db_step) : block("pwr_prefixer",
    				gr::io_signature::make(0,0,0),
    				gr::io_signature::make(0,0,0)),
    				d_phy_in(pmt::mp("phy_in")),
    				d_phy_out(pmt::mp("phy_out")),
    				d_pwr_in(pmt::mp("pwr_in")),
    				d_lowest_db(lowest_db),
    				d_db_step(db_step)
    	{
    		message_port_register_in(d_phy_in);
    		message_port_register_in(d_pwr_in);
    		message_port_register_out(d_phy_out);
    		set_msg_handler(d_pwr_in,boost::bind(&pwr_prefixer_impl::pwr_in,this,_1));
    		set_msg_handler(d_phy_in,boost::bind(&pwr_prefixer_impl::phy_in,this,_1));
    		d_cur_level = INIT;
    	}
    	void pwr_in(pmt::pmt_t msg)
    	{
    		// the input should be a calculated power tag with cdr=power (db)
    		gr::thread::scoped_lock guard(d_mutex);
    		pmt::pmt_t k = car(msg);
    		pmt::pmt_t v = cdr(msg);
    		assert(pmt::is_number(v));
    		float calc_pwr = pmt::to_float(v);
    		calc_pwr -= d_lowest_db;
    		if(calc_pwr<0){
                // use lowest
                d_cur_level = d_pwr_map[0];
    		}else{
    			int quantize_level = (int) floor(calc_pwr/d_db_step);
    			quantize_level = (quantize_level > d_pwr_map_size) ? 
                                    d_pwr_map_size-1 : quantize_level;
                d_cur_level = d_pwr_map[quantize_level];
    		}
    	}
    	void phy_in(pmt::pmt_t msg)
    	{
    		gr::thread::scoped_lock guard(d_mutex);
    		pmt::pmt_t k = car(msg);
    		pmt::pmt_t v = cdr(msg);
    		assert(pmt::is_blob(v));
    		size_t io(0);
    		const uint8_t* uvec = pmt::u8vector_elements(v,io);
    		if(io>126){
    			throw std::runtime_error("Payload length exceeds limit (126 bytes, 1 for power tag)");
    		}
            if(d_cur_level == INIT){
                d_buf[0] = 0x00;
            }else{
                d_buf[0] = (unsigned char) d_cur_level;    
            }
    		memcpy(d_buf+1,uvec,sizeof(char)*io);
            pmt::pmt_t blob = pmt::make_blob(d_buf,io+1);
            message_port_pub(d_phy_out,pmt::cons(pmt::PMT_NIL,blob));
    	}
    private:
    	const pmt::pmt_t d_phy_in;
    	const pmt::pmt_t d_pwr_in;
    	const pmt::pmt_t d_phy_out;
    	const float d_lowest_db;
    	const float d_db_step;
    	gr::thread::mutex d_mutex;
    	int d_cur_level;
    	unsigned char d_buf[256];
    };

    pwr_prefixer::sptr
    pwr_prefixer::make(float lowest_db,float db_step)
    {
    	return gnuradio::get_initial_sptr(new pwr_prefixer_impl(lowest_db,db_step));
    }

  } /* namespace pwr */
} /* namespace gr */

