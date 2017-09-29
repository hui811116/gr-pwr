/* -*- c++ -*- */

#define PWR_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "pwr_swig_doc.i"

%{
#include "pwr/pwr_tagger.h"
%}


%include "pwr/pwr_tagger.h"
GR_SWIG_BLOCK_MAGIC2(pwr, pwr_tagger);
