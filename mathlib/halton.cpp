// $Id: halton.cpp,v 1.2 2006-12-15 11:34:02 tgb Exp $

#include <halton.h>

HaltonSequenceGenerator_t::HaltonSequenceGenerator_t(int b)
{
	base=b;
	fbase=(float) b;
	seed=1;

}

float HaltonSequenceGenerator_t::GetElement(int elem)
{
	int tmpseed=seed;
	float ret=0.0;
	float base_inv=1.0/fbase;
	while(tmpseed)
	{
		int dig=tmpseed % base;
		ret+=((float) dig)*base_inv;
		base_inv/=fbase;
		tmpseed/=base;
	}
	return ret;
}
