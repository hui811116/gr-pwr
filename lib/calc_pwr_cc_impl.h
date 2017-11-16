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

#ifndef INCLUDED_PWR_CALC_PWR_CC_IMPL_H
#define INCLUDED_PWR_CALC_PWR_CC_IMPL_H

#include <pwr/calc_pwr_cc.h>

namespace gr {
  namespace pwr {

    class calc_pwr_cc_impl : public calc_pwr_cc
    {
     private:
      gr_complex d_acc_eng;
      //int d_acc_len;
      int d_acc_cnt;
      //bool d_state;
      bool d_do_report;
      bool d_finished;
      float d_pwr_db;
      float d_period;
      gr_complex* d_eg_buf;
      const size_t d_cap;

      gr::thread::mutex d_mutex;
      const pmt::pmt_t d_out_port;
      const pmt::pmt_t d_target_tag;
      boost::shared_ptr<gr::thread::thread> d_thread;
      boost::posix_time::ptime d_system_time;

      void run();

     public:
      calc_pwr_cc_impl(float period);
      ~calc_pwr_cc_impl();

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);

      void set_calc_len(int aclen);
      int calc_len() const;

      bool start();
      bool stop();
    };

  } // namespace pwr
} // namespace gr

#endif /* INCLUDED_PWR_CALC_PWR_CC_IMPL_H */

