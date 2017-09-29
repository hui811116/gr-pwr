/* -*- c++ -*- */



#ifndef INCLUDED_PWR_PWR_TAGGER_H
#define INCLUDED_PWR_PWR_TAGGER_H

#include <pwr/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace pwr {

    /*!
     * \brief <+description+>
     *
     */
    class PWR_API pwr_tagger : virtual public block
    {
    public:
      typedef boost::shared_ptr<pwr_tagger> sptr;
      static sptr make(float period);
    };

  } // namespace pwr
} // namespace gr

#endif /* INCLUDED_PWR_PWR_TAGGER_H */

