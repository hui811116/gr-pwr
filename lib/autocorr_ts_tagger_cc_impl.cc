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
#include "autocorr_ts_tagger_cc_impl.h"
#include <algorithm>
#include <gnuradio/math.h>

namespace gr {
  namespace pwr {
    #define GAPLEN 512
    #define VALIDLEN 128
    autocorr_ts_tagger_cc::sptr
    autocorr_ts_tagger_cc::make(float threshold, int delay)
    {
      return gnuradio::get_initial_sptr
        (new autocorr_ts_tagger_cc_impl(threshold,delay));
    }

    /*
     * The private constructor
     */
    static int ios [] ={sizeof(gr_complex),sizeof(gr_complex),sizeof(float)};
    static std::vector<int> iosig(ios,ios+sizeof(ios)/sizeof(int));

    autocorr_ts_tagger_cc_impl::autocorr_ts_tagger_cc_impl(float threshold, int delay)
      : gr::block("autocorr_ts_tagger_cc",
              gr::io_signature::makev(3, 3, iosig),
              gr::io_signature::make(1, 1, sizeof(gr_complex))),
              d_gap_len(GAPLEN),
              d_valid_len(VALIDLEN),
              d_auto_key(pmt::intern("auto_tag")),
              d_this_block(pmt::intern(alias()))
    {
      if(threshold>1 || threshold <= 0 || delay<0){
        throw std::invalid_argument("Invalid arguments for auto_ts_tagger");
      }
      d_threshold = threshold;
      d_delay = delay;
      d_auto_cnt =0;
      d_gap_cnt=0;
      set_tag_propagation_policy(TPP_DONT);
    }

    /*
     * Our virtual destructor.
     */
    autocorr_ts_tagger_cc_impl::~autocorr_ts_tagger_cc_impl()
    {
    }

    void
    autocorr_ts_tagger_cc_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      for(int i=0;i<ninput_items_required.size();++i)
        ninput_items_required[i] = noutput_items;
    }

    int
    autocorr_ts_tagger_cc_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const gr_complex *in = (const gr_complex *) input_items[0];
      const gr_complex *corr=(const gr_complex *) input_items[1];
      const float * mag = (const float *) input_items[2];
      gr_complex *out = (gr_complex *) output_items[0];
      int nout =0;
      int nin = std::min(ninput_items[0], std::min(ninput_items[1], ninput_items[2]));
      int count =0;
      const uint64_t nwrite = nitems_written(0);
      
      while(count<nin && nout<noutput_items){
        if(mag[count]> d_threshold){
          d_auto_cnt++;
          if(d_auto_cnt==d_valid_len){
            // avoid generating too many tags
            if(d_gap_cnt>=d_gap_len){
              float cfo = arg(corr[count])/(float)d_delay;
              add_item_tag(0,nwrite+count,d_auto_key,pmt::from_float(cfo),d_this_block);
              d_gap_cnt=0;
            }
            d_auto_cnt=0;
          }
        }else{
          d_auto_cnt=0;
        }
        out[nout++] = in[count++];
        d_gap_cnt++;
      }

      consume_each (count);
      return nout;
    }

  } /* namespace pwr */
} /* namespace gr */

