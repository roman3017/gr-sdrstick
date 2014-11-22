/* -*- c++ -*- */
/* 
 * Copyright 2014 <+YOU OR YOUR COMPANY+>.
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


#ifndef INCLUDED_SDRSTICK_SDRSTICK_SOURCE_S_H
#define INCLUDED_SDRSTICK_SDRSTICK_SOURCE_S_H

#include <sdrstick/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace sdrstick {

    /*!
     * \brief <+description of block+>
     * \ingroup sdrstick
     *
     */
    class SDRSTICK_API sdrstick_source_s : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<sdrstick_source_s> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of sdrstick::sdrstick_source_s.
       *
       * To avoid accidental use of raw pointers, sdrstick::sdrstick_source_s's
       * constructor is in a private implementation
       * class. sdrstick::sdrstick_source_s::make is the public interface for
       * creating new instances.
       */
       static sptr make(size_t itemsize,
                         const char *hostip,
                         const char *sipaddr,
                         int port,
                         int sr_idx);


         
        /*! \brief enter l.o. frequency in Hz */
        virtual void set_freq(int freq) = 0;    
            
        /*! \brief chose sample rate (0/1 = high/low) */
         virtual void set_samp_rate(int sr_idx) = 0;
    };

  } // namespace sdrstick
} // namespace gr

#endif /* INCLUDED_SDRSTICK_SDRSTICK_SOURCE_S_H */

