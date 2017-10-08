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


#ifndef INCLUDED_PWR_PWR_CTRL_H
#define INCLUDED_PWR_PWR_CTRL_H

#include <pwr/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace pwr {

    /*!
     * \brief <+description+>
     *
     */
    class PWR_API pwr_ctrl : virtual public block
    {
    public:
      typedef boost::shared_ptr<pwr_ctrl> sptr;
      static sptr make();
    };

  } // namespace pwr
} // namespace gr

#endif /* INCLUDED_PWR_PWR_CTRL_H */

