/* -*- c++ -*- */
/* 
 * Copyright 2014 Zephyr Engineering, Inc..
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

#ifndef INCLUDED_SDRSTICK_SDRSTICK_SOURCE_S_IMPL_H
#define INCLUDED_SDRSTICK_SDRSTICK_SOURCE_S_IMPL_H

#include <sdrstick/sdrstick_source_s.h>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <gnuradio/thread/thread.h>

namespace gr {
  namespace sdrstick {

    class sdrstick_source_s_impl : public sdrstick_source_s
    {
     private:
        size_t  d_itemsize;
        int     d_payload_size; // maximum transmission unit (packet length)
        bool    d_eof;          // look for an EOF signal
        bool    d_wait;         // wait if data if not immediately available
        int     d_socket;       // handle to socket
        char 	*d_temp_buff; 	// hold buffer between calls
        ssize_t d_residual;     // hold information about number of bytes stored in residbuf
        size_t 	d_temp_offset; 	// point to temp buffer location offset
        int 	d_pktnum;		// track incoming packet number to detect loss of data
        int 	d_pktcount;	    // used to locate packet header
        int 	d_cmd_socket;	// handle to command socket
        bool    d_connected;    // are we connected?
        int     d_product;      // product code
 


     public:
        sdrstick_source_s_impl(size_t itemsize,
                               const char *hostip,
                               const char *sipaddr,
                               int port,
                               int sr_idx
                               );

        ~sdrstick_source_s_impl();


      int is_error(int);
      void report_error(const char *,const char *);
      void set_freq(int freq);
      void set_run(void);
      void set_stop(void);
      void set_samp_rate(int sr_idx);
      unsigned int get_fwrev(void);
      unsigned int get_product(void);

      // Where all the action really happens
      int work(int noutput_items,
	       gr_vector_const_void_star &input_items,
	       gr_vector_void_star &output_items);
    };

  } // namespace sdrstick
} // namespace gr

#endif /* INCLUDED_SDRSTICK_SDRSTICK_SOURCE_S_IMPL_H */

