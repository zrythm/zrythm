#ifndef FILTERS_H
#define FILTERS_H

#include <cmath>


// http://www.musicdsp.org/archive.php?classid=0#128
class CFxRbjFilter
{
public:

	CFxRbjFilter()
	{
		// reset filter coeffs
		b0a0=b1a0=b2a0=a1a0=a2a0=0.0;

		// reset in/out history
		ou1=ou2=in1=in2=0.0f;
	};

	float run(float in0)
	{
		// filter
		float yn = b0a0*in0 + b1a0*in1 + b2a0*in2 - a1a0*ou1 - a2a0*ou2;

		if(!std::isnormal(yn)) yn= ou1;

		// push in/out buffers
		in2=in1;
		in1=in0;
		ou2=ou1;
		ou1=yn;

		// return output
		return yn;
	};

	void calc_filter_coeffs(int const type, double frequency, double const sample_rate, double q,
							double const db_gain, bool q_is_bandwidth)
	{
		// temp pi
		double const temp_pi=3.1415926535897932384626433832795;

		// temp coef vars
		double alpha,a0,a1,a2,b0,b1,b2;

		const float min_cutoff = 10.0f;
		const float max_cutoff = sample_rate * 0.45f;
		if(frequency<min_cutoff) frequency= min_cutoff;
		if(frequency>max_cutoff) frequency= max_cutoff;

		// peaking, lowshelf and hishelf
		if(type>=6)
		{
			double const A		=	pow(10.0,(db_gain/40.0));
			double const omega	=	2.0*temp_pi*frequency/sample_rate;
			double const tsin	=	sin(omega);
			double const tcos	=	cos(omega);

			if(q_is_bandwidth)
			alpha=tsin*sinh(log(2.0)/2.0*q*omega/tsin);
			else
			alpha=tsin/(2.0*q);

			double const beta	=	sqrt(A)/q;

			// peaking
			if(type==6)
			{
				b0=float(1.0+alpha*A);
				b1=float(-2.0*tcos);
				b2=float(1.0-alpha*A);
				a0=float(1.0+alpha/A);
				a1=float(-2.0*tcos);
				a2=float(1.0-alpha/A);
			}

			// lowshelf
			if(type==7)
			{
				b0=float(A*((A+1.0)-(A-1.0)*tcos+beta*tsin));
				b1=float(2.0*A*((A-1.0)-(A+1.0)*tcos));
				b2=float(A*((A+1.0)-(A-1.0)*tcos-beta*tsin));
				a0=float((A+1.0)+(A-1.0)*tcos+beta*tsin);
				a1=float(-2.0*((A-1.0)+(A+1.0)*tcos));
				a2=float((A+1.0)+(A-1.0)*tcos-beta*tsin);
			}

			// hishelf
			if(type==8)
			{
				b0=float(A*((A+1.0)+(A-1.0)*tcos+beta*tsin));
				b1=float(-2.0*A*((A-1.0)+(A+1.0)*tcos));
				b2=float(A*((A+1.0)+(A-1.0)*tcos-beta*tsin));
				a0=float((A+1.0)-(A-1.0)*tcos+beta*tsin);
				a1=float(2.0*((A-1.0)-(A+1.0)*tcos));
				a2=float((A+1.0)-(A-1.0)*tcos-beta*tsin);
			}
		}
		else
		{
			// other filters
			double const omega	=	2.0*temp_pi*frequency/sample_rate;
			double const tsin	=	sin(omega);
			double const tcos	=	cos(omega);

			if(q<0.0001) q= 0.0001;

			if(q_is_bandwidth)
				alpha=tsin*sinh(log(2.0)/2.0*q*omega/tsin);
			else
				alpha=tsin/(2.0*q);

			// lowpass
			if(type==0)
			{
				b0=(1.0-tcos)/2.0;
				b1=1.0-tcos;
				b2=(1.0-tcos)/2.0;
				a0=1.0+alpha;
				a1=-2.0*tcos;
				a2=1.0-alpha;
			}

			// hipass
			if(type==1)
			{
				b0=(1.0+tcos)/2.0;
				b1=-(1.0+tcos);
				b2=(1.0+tcos)/2.0;
				a0=1.0+ alpha;
				a1=-2.0*tcos;
				a2=1.0-alpha;
			}

			// bandpass csg
			if(type==2)
			{
				b0=tsin/2.0;
				b1=0.0;
			    b2=-tsin/2;
				a0=1.0+alpha;
				a1=-2.0*tcos;
				a2=1.0-alpha;
			}

			// bandpass czpg
			if(type==3)
			{
				b0=alpha;
				b1=0.0;
				b2=-alpha;
				a0=1.0+alpha;
				a1=-2.0*tcos;
				a2=1.0-alpha;
			}

			// notch
			if(type==4)
			{
				b0=1.0;
				b1=-2.0*tcos;
				b2=1.0;
				a0=1.0+alpha;
				a1=-2.0*tcos;
				a2=1.0-alpha;
			}

			// allpass
			if(type==5)
			{
				b0=1.0-alpha;
				b1=-2.0*tcos;
				b2=1.0+alpha;
				a0=1.0+alpha;
				a1=-2.0*tcos;
				a2=1.0-alpha;
			}
		}

		// set filter coeffs
		b0a0=float(b0/a0);
		b1a0=float(b1/a0);
		b2a0=float(b2/a0);
		a1a0=float(a1/a0);
		a2a0=float(a2/a0);
	};

	union
	{
		float params[5];
		struct
		{
			// filter coeffs
			float b0a0,b1a0,b2a0,a1a0,a2a0;
		};
	};

	// in/out history
	float ou1,ou2,in1,in2;
};


// http://www.musicdsp.org/archive.php?classid=0#227
#define BUDDA_Q_SCALE 6.f
class CFilterButterworth24db
{
	public:
		typedef double float_type;

		CFilterButterworth24db(void)
		{
			this->history1 = 0.f;
			this->history2 = 0.f;
			this->history3 = 0.f;
			this->history4 = 0.f;

			this->SetSampleRate(44100.f);
			this->Set(22050.f, 0.0);
		}

		void SetSampleRate(float fs)
		{
			float_type pi = 4.f * atanf(1.f);

			this->t0 = 4.f * fs * fs;
			this->t1 = 8.f * fs * fs;
			this->t2 = 2.f * fs;
			this->t3 = pi / fs;

			this->min_cutoff = fs * 0.01f;
			this->max_cutoff = fs * 0.45f;
		}

		void Set(float_type cutoff, float_type q)
		{
			if (cutoff < this->min_cutoff)
				cutoff = this->min_cutoff;
			else if(cutoff > this->max_cutoff)
				cutoff = this->max_cutoff;

			if(q < 0.f)
				q = 0.f;
			else if(q > 1.f)
				q = 1.f;

			float wp = this->t2 * tanf(this->t3 * cutoff);
			float bd, bd_tmp, b1, b2;

			q *= BUDDA_Q_SCALE;
			q += 1.f;

			b1 = (0.765367f / q) / wp;
			b2 = 1.f / (wp * wp);

			bd_tmp = this->t0 * b2 + 1.f;

			bd = 1.f / (bd_tmp + this->t2 * b1);

			this->gain = bd;

			this->coef2 = (2.f - this->t1 * b2);

			this->coef0 = this->coef2 * bd;
			this->coef1 = (bd_tmp - this->t2 * b1) * bd;

			b1 = (1.847759f / q) / wp;

			bd = 1.f / (bd_tmp + this->t2 * b1);

			this->gain *= bd;
			this->coef2 *= bd;
			this->coef3 = (bd_tmp - this->t2 * b1) * bd;
		}

		float_type Run(float_type input)
		{
			float_type output = input * this->gain;
			float_type new_hist;

			output -= this->history1 * this->coef0;
			new_hist = output - this->history2 * this->coef1;

			output = new_hist + this->history1 * 2.f;
			output += this->history2;

			this->history2 = this->history1;
			this->history1 = new_hist;

			output -= this->history3 * this->coef2;
			new_hist = output - this->history4 * this->coef3;

			output = new_hist + this->history3 * 2.f;
			output += this->history4;

			this->history4 = this->history3;
			this->history3 = new_hist;

			return output;
		}

		union
		{
			float_type params[9];
			struct
			{
				float_type coef0, coef1, coef2, coef3;
				float_type gain;
				float_type t0, t1, t2, t3;
			};
		};
		float_type history1, history2, history3, history4;
		float_type min_cutoff, max_cutoff;
};


class filter3band
{
	public:
		union
		{
			struct
			{ float cut, reso, l, b, h; };
			float params[5];
		};

		filter3band() { low= high= band= reso= 0; l= b= h= cut= 0.5; }

		double run(double val)
		{
			low+= cut * band;
			high = val - low - (1 - reso) * band;
			band+= cut * high;
			return low*l + band*b + high*h;
		}

	private:
		double low, high, band;
};

template <typename T, int N> struct multifilter
{
	T filters[N];

	T &operator[] (int idx) { return filters[idx]; }

	void updateparams()
	{
		for(int i= 1; i<N; i++)
			memcpy(filters[i].params, filters[0].params, sizeof(filters[0].params));
	}

	double run(double val, int n= N)
	{
		for(int i= 0; i<n; i++)
			val= filters[i].run(val);
		return val;
	}
};



template <typename T> struct velocityfilter
{
	public:
		velocityfilter(): velocity(0.5), inertia(0.5), value(0), speed(0)
		{ }

		void setparams(T velocity, T inertia)
		{
			this->velocity= velocity; this->inertia= inertia;
		}

		T run(T newval, T time)
		{
			//T oldval= value, oldspd= speed;
			speed+= (newval-value)*time * velocity;
			value+= speed*time;
			speed-= speed*(inertia*time);
			if(!std::isnormal(speed)) speed= 0;
			if(!std::isnormal(value)) value= 0;
			return value;
		}

		T getvalue() { return value; }
		void setvalue(T val) { value= val; }
		T getspeed() { return speed; }
		void setspeed(T val) { speed= val; }

	private:
		T velocity, inertia;
		T value, speed;
};


template <int N> struct bandpass
{
	multifilter<CFxRbjFilter, N> high, low;

	bandpass()
	{
	}

	void setparams(float sampling_rate, float freq, float bandwidth, float q)
	{
//	void calc_filter_coeffs(int const type, double frequency, double const sample_rate, double q,
//							double const db_gain, bool q_is_bandwidth)
		const float min_freq= 50.0;
		low[0].calc_filter_coeffs(0, (freq+bandwidth<min_freq? min_freq: freq+bandwidth),
								  sampling_rate, q, 0, true);
		low.updateparams();
		high[0].calc_filter_coeffs(1, (freq-bandwidth<min_freq? min_freq: freq-bandwidth),
								   sampling_rate, q, 0, true);
		high.updateparams();
	}

	double run(double value, int n= N)
	{
		double v= low.run(value, n);
		return high.run(v, n);
	}
};


#endif // FILTERS_H
