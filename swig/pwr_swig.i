/* -*- c++ -*- */

#define PWR_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "pwr_swig_doc.i"

%{
#include "pwr/pwr_tagger.h"
#include "pwr/calc_pwr_cc.h"
#include "pwr/pwr_ctrl.h"
#include "pwr/pwr_sender.h"
%}


%include "pwr/pwr_tagger.h"
GR_SWIG_BLOCK_MAGIC2(pwr, pwr_tagger);
%include "pwr/calc_pwr_cc.h"
GR_SWIG_BLOCK_MAGIC2(pwr, calc_pwr_cc);
%include "pwr/pwr_ctrl.h"
GR_SWIG_BLOBK_MAGIC2(pwr, pwr_ctrl);
%include "pwr/pwr_sender.h"
GR_SWIG_BLOBK_MAGIC2(pwr, pwr_sender);

