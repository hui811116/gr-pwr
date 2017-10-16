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

namespace gr {
  namespace pwr {
    #define d_debug true
    #define dout d_debug && std::cout
    static float d_update_period = 1000; // 1.0 milliseconds
    calc_pwr_cc::sptr
    calc_pwr_cc::make(int aclen,const std::string& tagname)
    {
      return gnuradio::get_initial_sptr
        (new calc_pwr_cc_impl(aclen, tagname));
    }

    /*
     * The private constructor
     */
    calc_pwr_cc_impl::calc_pwr_cc_impl(int aclen, const std::string& tagname)
      : gr::sync_block("calc_pwr_cc",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(1, 1, sizeof(gr_complex))),
              d_out_port(pmt::mp("msg_out")),
              d_target_tag(pmt::intern(tagname))
    {
      message_port_register_out(d_out_port);
      set_calc_len(aclen);
    }

    /*
     * Our virtual destructor.
     */
    calc_pwr_cc_impl::~calc_pwr_cc_impl()
    {
    }

    void calc_pwr_cc_impl::set_calc_len(int aclen)
    {
      if(aclen<0){
        throw std::invalid_argument("Accumulation length should be positive, please reset...");
      }
      d_acc_len = aclen;
      d_acc_cnt=0;
      d_state = false;
      d_acc_eng = 0;
      d_do_report = false;
    }
    int calc_pwr_cc_impl::calc_len() const
    {
      return d_acc_len;
    }

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
        boost::this_thread::sleep(boost::posix_time::milliseconds(d_update_period));
        if(!d_finished){
          return;
        }
        if(d_do_report){
          d_do_report = false;
          std::cout<<"<Calc Power>output calculated pwr db"<<d_pwr_db<<std::endl;
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

      std::vector<tag_t> tags;
      get_tags_in_window(tags,0,0,noutput_items,d_target_tag);
      int count =0;
      while(count<noutput_items){
        if(!tags.empty()){
          int offset = tags[0].offset - nitems_read(0);
          if(offset == count){
            if(!d_state){
              d_state = true;
              d_acc_eng=0;
              d_acc_cnt =0;
            }
            tags.erase(tags.begin());
          }
        }
        d_acc_eng += (d_state)?  std::norm(in[count]) : 0;
        d_acc_cnt = (d_state)? d_acc_cnt+1 : 0;
        if(d_acc_cnt == d_acc_len){
          float temp_eng = (float)d_acc_eng/(double)d_acc_len;
          if(temp_eng>0){
            d_pwr_db = 10.0 * std::log10(temp_eng);
            d_do_report = true;
          }
          d_acc_cnt =0;
          d_acc_cnt=0;
          d_state = false;
        }
        count++;
      }
      std::memcpy(out,in,sizeof(gr_complex)*noutput_items);
      return noutput_items;
    }

  } /* namespace pwr */
} /* namespace gr */

