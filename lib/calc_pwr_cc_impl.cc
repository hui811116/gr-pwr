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
#include "calc_pwr_cc_impl.h"
#include <cstring>
#include <cmath>
#include <volk/volk.h>

namespace gr {
  namespace pwr {
    #define d_debug true
    #define dout d_debug && std::cout
    calc_pwr_cc::sptr
    calc_pwr_cc::make(float period)
    {
      return gnuradio::get_initial_sptr
        (new calc_pwr_cc_impl(period));
    }

    /*
     * The private constructor
     */
    calc_pwr_cc_impl::calc_pwr_cc_impl(float period)
      : gr::sync_block("calc_pwr_cc",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(1, 1, sizeof(gr_complex))),
              d_out_port(pmt::mp("msg_out")),
              d_cap(8192*32)
    {
      message_port_register_out(d_out_port);
      //set_calc_len(aclen);
      if(period<0){
        throw std::invalid_argument("Period (ms) should be positive number");
      }
      d_period = period;
      set_max_noutput_items(d_cap);
      d_eg_buf = (gr_complex*) volk_malloc(sizeof(gr_complex)*d_cap,volk_get_alignment());
      d_acc_eng = gr_complex(0,0);
    }

    /*
     * Our virtual destructor.
     */
    calc_pwr_cc_impl::~calc_pwr_cc_impl()
    {
      volk_free(d_eg_buf);
    }
    /*
    void calc_pwr_cc_impl::set_calc_len(int aclen)
    {
      if(aclen<0){
        throw std::invalid_argument("Accumulation length should be positive, please reset...");
      }
      d_acc_len = aclen;
      d_acc_cnt=0;
      d_acc_eng = gr_complex(0,0);
      d_do_report = false;
    }
    int calc_pwr_cc_impl::calc_len() const
    {
      return d_acc_len;
    }
    */
    bool calc_pwr_cc_impl::start()
    {
      d_finished = false;
      d_system_time = boost::posix_time::microsec_clock::local_time();
      d_thread = boost::shared_ptr<gr::thread::thread>
        (new gr::thread::thread(&calc_pwr_cc_impl::run,this));
      return block::start();
    }
    bool calc_pwr_cc_impl::stop()
    {
      d_finished = false;
      d_thread->interrupt();
      d_thread->join();
      return block::stop();
    }
    void calc_pwr_cc_impl::run()
    {
      while(true){
        boost::this_thread::sleep(boost::posix_time::milliseconds(d_period));
        if(!d_finished){
          return;
        }
        if(d_do_report){
          d_do_report = false;
          //dout<<"<Calc Power>output calculated pwr db"<<d_pwr_db<<std::endl;
          d_pwr_db = std::log(real(d_acc_eng)/(float)d_acc_cnt);
          d_acc_cnt=0;
          d_acc_eng = gr_complex(0,0);
          message_port_pub(d_out_port,pmt::cons(pmt::PMT_NIL, pmt::from_float(d_pwr_db)));
        }
      }
    }

    int
    calc_pwr_cc_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const gr_complex *in = (const gr_complex *) input_items[0];
      gr_complex *out = (gr_complex *) output_items[0];
      volk_32fc_x2_conjugate_dot_prod_32fc(d_eg_buf,in,in,noutput_items);
      int count =0;
      while(count<noutput_items){
        d_acc_eng += d_eg_buf[count++];
        //d_acc_eng += std::norm(in[count]);
        d_acc_cnt++;
      }
      std::memcpy(out,in,sizeof(gr_complex)*noutput_items);
      return noutput_items;
    }

  } /* namespace pwr */
} /* namespace gr */

