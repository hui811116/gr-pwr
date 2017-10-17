/* -*- c++ -*- */



#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include <pwr/pwr_tagger.h>
#include <gnuradio/block_detail.h>

namespace gr {
  namespace pwr {
  	#define d_debug false
  	#define dout d_debug && std::cout
  	static const pmt::pmt_t d_gtag= pmt::intern("gain");
    class pwr_tagger_impl : public pwr_tagger
    {
    public:
    	pwr_tagger_impl(float period): block("pwr_tagger",
    		gr::io_signature::make(0,0,0),
    		gr::io_signature::make(0,0,0)),
    		d_in_port(pmt::mp("pwr_in")),
    		d_out_port(pmt::mp("pwr_out")),
    		d_curr_pwr(0.1),d_next_pwr(0.1)
    	{
    		message_port_register_in(d_in_port);
    		message_port_register_out(d_out_port);
    		set_msg_handler(d_in_port,boost::bind(&pwr_tagger_impl::msg_in,this,_1));
    		if(period < 0.0){
    			d_period = 1.0;
    		}else if(period ==0){
                // supported and termed "immediate mode"
                d_period = 0;
            }else{
    			d_period = (double)period;
    		}
    	}
    	~pwr_tagger_impl()
    	{

    	}
    	bool start()
    	{
    		d_finished = false;
    		d_sys_time = boost::posix_time::second_clock::local_time();
    		d_thread = boost::shared_ptr<gr::thread::thread>
    		(new gr::thread::thread(boost::bind(&pwr_tagger_impl::run,this)));
    		return block::start();
    	}
    	bool stop()
    	{
    		d_finished = true;
    		d_thread->interrupt();
    		d_thread->join();
    		return block::stop();
    	}
    	void msg_in(pmt::pmt_t msg)
    	{
    		gr::thread::scoped_lock guard(d_mutex);
    		pmt::pmt_t k = pmt::car(msg);
    		pmt::pmt_t v = pmt::cdr(msg);
    		if(pmt::is_real(v)){
    			double get_power = pmt::to_double(v);
    			if(get_power!=d_curr_pwr){
    				// can add some power checking function to avoid usrp gain overflow
    				d_next_pwr = get_power;
    				d_update = true;
    			}
    		}
    	}
    private:
    	void run()
    	{
    		while(!d_finished){
                if(d_period>0){
                    boost::this_thread::sleep(boost::posix_time::milliseconds(d_period));    
                }else{
                    // imm mode
                    gr::thread::scoped_lock lock(d_mutex);
                    d_imm_update.wait(lock);
                    lock.unlock();
                }
    			if(d_finished){
    				return;
    			}
    			boost::posix_time::time_duration diff = 
    			boost::posix_time::second_clock::local_time() - d_sys_time;
    			if(d_update){
    				d_update = false;
    				d_curr_pwr = d_next_pwr;
    				message_port_pub(d_out_port,pmt::cons(d_gtag,pmt::from_double(d_curr_pwr)));
    				dout<<"Update power tag for power = "<<d_curr_pwr<<" dB"
    				<<", system time:"<<diff.total_seconds()<<std::endl;
    			}
    		}
    	}
    	const pmt::pmt_t d_in_port;
    	const pmt::pmt_t d_out_port;
    	boost::shared_ptr<gr::thread::thread> d_thread;
    	boost::posix_time::ptime d_sys_time;
    	gr::thread::mutex d_mutex;
        gr::thread::condition_variable d_imm_update;
    	float d_period;
    	double d_curr_pwr;
    	double d_next_pwr;
    	bool d_finished;
    	bool d_update;

    };

    pwr_tagger::sptr
    pwr_tagger::make(float period)
    {
    	return gnuradio::get_initial_sptr(new pwr_tagger_impl(period));
    }

  } /* namespace pwr */
} /* namespace gr */

