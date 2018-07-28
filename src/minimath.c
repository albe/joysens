const float shift23=(1<<23);
const float OOshift23=1.0/(1<<23);	// = 1.1920929e-7 = 0,00000011920929

#define EPS 1.e-5f

float log2(float i)
{
	if (i>1.f-EPS && i<1.f+EPS)
		return 0.0f;
	float x;
	float y;
	//Compiler is picky about that line and doesn't compile it reliably into the wanted output
	//x=*(int *)&i;	// i = 0.33 = 00111110101010001111010111000011b = 1051260355 => x = 1051260350.0
	asm("cvt.s.w %0, %1\n"
		:"=f"(x):"f"(i));
	x*= OOshift23; //1/pow(2,23); => x = (float)125.3199999286515 = 125.32
	x=x-127;	// x = -1.68

	y=x-(int)x;		// x - trunc(x) = +-frac(x) => y = -1.68 - (-1) = -0.68
	y=((y<0?-y:y)-y*y)*0.346607f;	// y = (0.68 - 0.4624)*LogBodge = 0.2176*0.346607 = (float)0.0754216832 = 0.07542168
	return x+y;	// return (float)-1.60457832 = -1.6045784 :: log2( 0.33 ) = (float)-1.5994620704162712580875368683052 = -1.599462

	// absolute error = 0,00512
	// relative error = 0,3199%
}

float pow2(float i)
{
	if (i<EPS && i>-EPS)
		return 1.0f;
	if (i>64.f)
		return 3.4028235e38f;
	if (i<-64.f)
		return 0.0f;
	float x;
	float y=i-(int)i;
	y=((y<0?-y:y)-y*y)*0.33971f;

	x=i+127-y;
	x*= shift23; //pow(2,23);
	//*(int*)&x=(int)x;	// Compiler doesn't compile this reliably into the below
	asm("trunc.w.s %0, %1\n"
		:"=f"(x):"f"(x));
	return x;
}

float powf( float a, float b )
{
	if (a <= EPS) return 0.0f;
	return pow2(b*log2(a));
}
