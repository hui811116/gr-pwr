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


#ifndef INCLUDED_PWR_CALC_PWR_CC_H
#define INCLUDED_PWR_CALC_PWR_CC_H

#include <pwr/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace pwr {

    /*!
     * \brief <+description of block+>
     * \ingroup pwr
     *
     */
    class PWR_API calc_pwr_cc : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<calc_pwr_cc> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of pwr::calc_pwr_cc.
       *
       * To avoid accidental use of raw pointers, pwr::calc_pwr_cc's
       * constructor is in a private implementation
       * class. pwr::calc_pwr_cc::make is the public interface for
       * creating new instances.
       */
      static sptr make(int aclen, const std::string& tagname);

      virtual void set_calc_len(int aclen)=0;
      virtual int calc_len()const =0;
    };

  } // namespace pwr
} // namespace gr

#endif /* INCLUDED_PWR_CALC_PWR_CC_H */

