/* -*- c++ -*- */

#define SDRSTICK_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "sdrstick_swig_doc.i"

%{
#include "sdrstick/sdrstick_source_s.h"
%}


%include "sdrstick/sdrstick_source_s.h"
GR_SWIG_BLOCK_MAGIC2(sdrstick, sdrstick_source_s);
