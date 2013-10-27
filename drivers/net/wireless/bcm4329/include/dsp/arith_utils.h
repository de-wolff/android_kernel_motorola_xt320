#ifndef _ARITH_UTILS_H_
#define _ARITH_UTILS_H_


/***************************/
/*****    rounding     *****/
/***************************/

static INLINE cint16
C16_HWROUND(
    cint16 x,
    uint16 nfracbits
)
{
    cint16 z;

    z.i = HWROUND0( (x.i) , nfracbits );
    z.q = HWROUND0( (x.q) , nfracbits );
    return(z);
}

static INLINE cint32
C32_HWROUND(
    cint32 x,
    uint16 nfracbits
)
{
    cint32 z;

    z.i = HWROUND0( (x.i) , nfracbits );
    z.q = HWROUND0( (x.q) , nfracbits );
    return(z);
}

static INLINE cint64
C64_HWROUND(
    cint64 x,
    uint32 nfracbits
)
{
    cint64 z;

    z.i = HWROUND0( (int64) (x.i) , nfracbits );
    z.q = HWROUND0( (int64) (x.q) , nfracbits );
    return(z);
}

static INLINE cint32
C64_C32_HWROUND(
    cint64 x,
    uint32 nfracbits
)
{
    cint32 z;

    z.i = (int32)HWROUND0( (int64) (x.i) , nfracbits );
    z.q = (int32)HWROUND0( (int64) (x.q) , nfracbits );
    return(z);
}

static INLINE cint16
C32_C16_HWROUND(
    cint32 x,
    uint8 nfracbits
)
{
    cint16 z;

    z.i = (int16)HWROUND0( (int32) (x.i) , nfracbits );
    z.q = (int16)HWROUND0( (int32) (x.q) , nfracbits );
    return(z);
}


/***************************/
/*****   saturation    *****/
/***************************/

static INLINE cint16
C16_SAT(
    cint16 x,
    uint16 num_signed_bits
)
{

    cint16 z;

    z.i = LIMIT( x.i, -(1<<(num_signed_bits-1)), ((1<<(num_signed_bits-1))-1) );
    z.q = LIMIT( x.q, -(1<<(num_signed_bits-1)), ((1<<(num_signed_bits-1))-1) );
    return(z);

}

static INLINE cint32
C32_SAT(
    cint32 x,
    uint16 num_signed_bits
)
{

    cint32 z;

    z.i = LIMIT( x.i, -(1<<(num_signed_bits-1)), ((1<<(num_signed_bits-1))-1) );
    z.q = LIMIT( x.q, -(1<<(num_signed_bits-1)), ((1<<(num_signed_bits-1))-1) );
    return(z);

}

static INLINE cint16
C32_C16_SAT(
    cint32 x,
    uint16 num_signed_bits
)
{

    cint16 z;

    z.i = LIMIT( x.i, -(1<<(num_signed_bits-1)), ((1<<(num_signed_bits-1))-1) );
    z.q = LIMIT( x.q, -(1<<(num_signed_bits-1)), ((1<<(num_signed_bits-1))-1) );
    return(z);

}

static INLINE cint8
C16_C8_SAT(
    cint16 x,
    uint16 num_signed_bits
)
{

    cint8 z;

    z.i = LIMIT( x.i, -(1<<(num_signed_bits-1)), ((1<<(num_signed_bits-1))-1) );
    z.q = LIMIT( x.q, -(1<<(num_signed_bits-1)), ((1<<(num_signed_bits-1))-1) );
    return(z);

}

static INLINE cint64
C64_SAT(
    cint64 x,
    uint16 num_signed_bits
)
{

    cint64 z;

    z.i = LIMIT( (int64)x.i, -((int64)1<<(num_signed_bits-1)), (((int64)1<<(num_signed_bits-1))-1) );
    z.q = LIMIT( (int64)x.q, -((int64)1<<(num_signed_bits-1)), (((int64)1<<(num_signed_bits-1))-1) );
    return(z);

}

static INLINE cint32
C64_C32_SAT(
    cint64 x,
    uint16 num_signed_bits
)
{

    cint32 z;

    z.i = (int32) LIMIT(x.i, -(1<<(num_signed_bits-1)), ((1<<(num_signed_bits-1))-1) );
    z.q = (int32) LIMIT(x.q, -(1<<(num_signed_bits-1)), ((1<<(num_signed_bits-1))-1) );
    return(z);

}

static INLINE int64
S64_SAT(
    int64 x,
    uint16 num_signed_bits
)
{

    int64 z;

    z = LIMIT( (int64)x, -((int64)1<<(num_signed_bits-1)), (((int64)1<<(num_signed_bits-1))-1) );
    return(z);

}

static INLINE int32
S32_SAT(
    int32 x,
    uint16 num_signed_bits
)
{

    int32 z;

    z = LIMIT( (int32)x, -(1<<(num_signed_bits-1)), ((1<<(num_signed_bits-1))-1) );
    return(z);

}

static INLINE int16
S16_SAT(
    int16 x,
    uint16 num_signed_bits
)
{

    int16 z;

    z = LIMIT( (int16)x, -(1<<(num_signed_bits-1)), ((1<<(num_signed_bits-1))-1) );
    return(z);

}

static INLINE uint32
U32_SAT(
    uint32 x,
    uint16 num_bits
)
{

    uint32 z;

    z = LIMIT( (uint32)x, 0, (((uint32)1<<num_bits)-1) );
    return(z);

}

static INLINE uint64
U64_SAT(
    uint64 x,
    uint16 num_bits
)
{

    uint64 z;

    z = LIMIT( (uint64)x, 0, (((uint64)1<<num_bits)-1) );
    return(z);

}

static INLINE int32
R64_R32_SAT(
    int64 x,
    uint16 num_signed_bits
)
{
    int32 z;

    z = LIMIT( x, -(1<<(num_signed_bits-1)), ((1<<(num_signed_bits-1))-1) );

    return(z);
}


/***************************/
/*****    basic ops    *****/
/***************************/

static INLINE cint16
C_CONJ16(
    cint16 x
)
{
    cint16 z;

    z.i = x.i;
    z.q = -x.q;
    return(z);
}

static INLINE cint32
C_CONJ32(
    cint32 x
)
{
    cint32 z;

    z.i = x.i;
    z.q = -x.q;
    return(z);
}

static INLINE cint64
C_CONJ64(
    cint64 x
)
{
    cint64 z;

    (z.i) = (int64) x.i;
    (z.q) = - (int64) x.q;
    return(z);
}


static INLINE cint32
C_NEG32(
    cint32 x
)
{
    cint32 z;

    z.i = -x.i;
    z.q = -x.q;
    return(z);
}

static INLINE cint64
C_NEG64(
    cint64 x
)
{
    cint64 z;

    z.i = -x.i;
    z.q = -x.q;
    return(z);
}


static INLINE cint16
C_AVG16(
    cint16 x,
    cint16 y
)
{
    cint16 z;

    z.i = (int16)HWROUND0(x.i + y.i, 1);
    z.q = (int16)HWROUND0(x.q + y.q, 1);
    return(z);
}

static INLINE cint32
C_AVG32(
    cint32 x,
    cint32 y
)
{
    cint32 z;

    z.i = (int32)HWROUND0((int64)x.i + (int64)y.i, 1);
    z.q = (int32)HWROUND0((int64)x.q + (int64)y.q, 1);
    return(z);
}

static INLINE cint16
C_ADD16(
    cint16 x,
    cint16 y
)
{
    cint16 z;

    z.i = x.i + y.i;
    z.q = x.q + y.q;
    return(z);
}

static INLINE cint32
C_ADD16_C32(
    cint16 x,
    cint16 y
)
{
    cint32 z;

    z.i = x.i + y.i;
    z.q = x.q + y.q;
    return(z);
}

static INLINE cint64
C_ADD32_C64(
    cint32 x,
    cint32 y
)
{
    cint64 z;

    z.i = x.i + y.i;
    z.q = x.q + y.q;
    return(z);
}

static INLINE cint32
C16C32_C32_ADD(
    cint16 x,
    cint32 y
)
{
    cint32 z;

    z.i = (int32)x.i + y.i;
    z.q = (int32)x.q + y.q;
    return(z);
}

static INLINE cint64
C32C64_C64_ADD(
    cint32 x,
    cint64 y
)
{
    cint64 z;

    z.i = (int64)x.i + y.i;
    z.q = (int64)x.q + y.q;
    return(z);
}


static INLINE cint16
C_SUB16(
    cint16 x,
    cint16 y
)
{
    cint16 z;

    z.i = x.i - y.i;
    z.q = x.q - y.q;
    return(z);
}

static INLINE cint32
C_SUB16_C32(
    cint16 x,
    cint16 y
)
{
    cint32 z;

    z.i = x.i - y.i;
    z.q = x.q - y.q;
    return(z);
}

static INLINE cint32
C_ADD32(
    cint32 x,
    cint32 y
)
{
    cint32 z;

    z.i = x.i + y.i;
    z.q = x.q + y.q;
    return(z);
}

static INLINE cint32
C_SUB32(
    cint32 x,
    cint32 y
)
{
    cint32 z;

    z.i = x.i - y.i;
    z.q = x.q - y.q;
    return(z);
}

static INLINE cint64
C_ADD64(
    cint64 x,
    cint64 y
)
{
    cint64 z;

    z.i = x.i + y.i;
    z.q = x.q + y.q;
    return(z);
}

static INLINE cint64
C_SUB64(
    cint64 x,
    cint64 y
)
{
    cint64 z;

    z.i = x.i - y.i;
    z.q = x.q - y.q;
    return(z);
}


/***************************/
/*****  mult, mag_sq   *****/
/***************************/

static INLINE uint32
C_MAG2_16_32(
    cint16 x,
    uint16 nfracbits
)
{
    return(HWROUND0((x.i * x.i + x.q * x.q), nfracbits));
}

static INLINE uint64
C_MAG2_32_64(
    cint32 x,
    uint16 nfracbits
)
{
    return(HWROUND0(((int64)x.i * (int64)x.i + (int64)x.q * (int64)x.q), nfracbits));
}

static INLINE cint32
C16C16_C32_MPY(
    cint16 x,
    cint16 y
)
{
    cint32 z;

    z.i = x.i*y.i - x.q*y.q;
    z.q = x.i*y.q + x.q*y.i;
    return(z);
}

static INLINE cint32
C16C16_C32_EMPY(
    cint16 x,
    cint16 y,
    uint16 nfracbits
)
{
    int16 s1, s2, s3;
    int32 p1, p2, p3;
    cint32 z;

    s1 = x.i - x.q;
    s2 = y.i - y.q;
    s3 = y.i + y.q;

    p1 = s1 * y.q;
    p2 = s2 * x.i;
    p3 = s3 * x.q;

    z.i = HWROUND0(p1 + p2, nfracbits);
    z.q = HWROUND0(p1 + p3, nfracbits);
    return(z);
}



static INLINE cint32
C16C8_C32_MPY(
    cint16 x,
    cint8  y
)
{
    cint32 z;

    z.i = x.i * (int16)y.i - x.q * (int16)y.q;
    z.q = x.i * (int16)y.q + x.q * (int16)y.i;
    return(z);
}

static INLINE cint32
C16C_CONJ16_C32_MPY(
    cint16 x,
    cint16 y
)
{
    cint32 z;
    z.i =  x.i*y.i + x.q*y.q;
    z.q = -x.i*y.q + x.q*y.i;
    return(z);
}

static INLINE cint32
C32C_CONJ16_C32_MPY(
    cint32 x,
    cint16 y
)
{
    cint32 z;
    z.i =  x.i*y.i + x.q*y.q;
    z.q = -x.i*y.q + x.q*y.i;
    return(z);
}

static INLINE cint32
C16C_CONJ32_C32_MPY(
    cint16 x,
    cint32 y
)
{
    cint32 z;
    z.i =  x.i*y.i + x.q*y.q;
    z.q = -x.i*y.q + x.q*y.i;
    return(z);
}

static INLINE cint64
C32C_CONJ16_C64_MPY(
    cint32 x,
    cint16 y
)
{
    cint64 z;
    z.i = (int64)(  (int64)x.i * (int64)y.i + (int64)x.q * (int64)y.q);
    z.q = (int64)(- (int64)x.i * (int64)y.q + (int64)x.q * (int64)y.i);
    return(z);
}

static INLINE cint64
C16C32_C64_MPY(
    cint16 x,
    cint32 y
)
{
    cint64 z;

    z.i = (int64)((int64)x.i * (int64)y.i - (int64)x.q * (int64)y.q);
    z.q = (int64)((int64)x.i * (int64)y.q + (int64)x.q * (int64)y.i);
    return(z);
}


static INLINE cint32
R8C16_C32_MPY(
    int8  x,
    cint16 y,
    uint16 num_frac_bits
)
{
    cint32 z;

    z.i = (int32)HWROUND0( (int32)x * (int32)y.i , num_frac_bits);
    z.q = (int32)HWROUND0( (int32)x * (int32)y.q , num_frac_bits);
    return(z);
}

static INLINE cint32
R8C32_C32_MPY(
    int8  x,
    cint32 y,
    uint16 num_frac_bits
)
{
    cint32 z;

    z.i = (int32)HWROUND0( (int32)x * y.i , num_frac_bits);
    z.q = (int32)HWROUND0( (int32)x * y.q , num_frac_bits);
    return(z);
}


static INLINE cint32
R16C16_C32_MPY(
    int16  x,
    cint16 y,
    uint16 num_frac_bits
)
{
    cint32 z;

    z.i = (int32)HWROUND0( (int32)x * (int32)y.i , num_frac_bits);
    z.q = (int32)HWROUND0( (int32)x * (int32)y.q , num_frac_bits);
    return(z);
}

static INLINE cint64
R16C32_C64_MPY(
    int16 x,
    cint32 y,
    uint16 num_frac_bits
)
{
    cint64 z;

    z.i = (int64)HWROUND0( (int64)x * (int64)y.i , num_frac_bits);
    z.q = (int64)HWROUND0( (int64)x * (int64)y.q , num_frac_bits);
    return(z);
}

static INLINE cint64
R32C16_C64_MPY(
    int32 x,
    cint16 y,
    uint16 num_frac_bits
)
{
    cint64 z;

    z.i = (int64)HWROUND0( (int64)x * (int64)y.i , num_frac_bits);
    z.q = (int64)HWROUND0( (int64)x * (int64)y.q , num_frac_bits);
    return(z);
}

static INLINE cint64
R32C32_C64_MPY(
    int32 x,
    cint32 y,
    uint16 num_frac_bits
)
{
    cint64 z;

    z.i = (int64)HWROUND0( (int64)x * (int64)y.i , num_frac_bits);
    z.q = (int64)HWROUND0( (int64)x * (int64)y.q , num_frac_bits);
    return(z);
}


static INLINE cint64
C32C32_C64_MPY(
    cint32 x,
    cint32 y
)
{
    cint64 z;

    z.i = (int64)((int64)x.i * (int64)y.i - (int64)x.q * (int64)y.q);
    z.q = (int64)((int64)x.i * (int64)y.q + (int64)x.q * (int64)y.i);
    return(z);
}

static INLINE void
C16_MPYA(
    cint16 x,
    cint16 y,
    cint16 *acc,
    uint16 nfracbits
)
{
    acc->i += HWROUND0(x.i * y.i - x.q * y.q, nfracbits);
    acc->q += HWROUND0(x.i * y.q + x.q * y.i, nfracbits);
}

static INLINE void
C16_MPYS(
    cint16 x,
    cint16 y,
    cint16 *acc,
    uint16 nfracbits
)
{
    acc->i -= HWROUND0(x.i * y.i - x.q * y.q, nfracbits);
    acc->q -= HWROUND0(x.i * y.q + x.q * y.i, nfracbits);
}

static INLINE void
C16_MPYA_32(
    cint16 x,
    cint16 y,
    cint32 *acc,
    uint16 nfracbits
)
{
    acc->i += HWROUND0(x.i * y.i - x.q * y.q, nfracbits);
    acc->q += HWROUND0(x.i * y.q + x.q * y.i, nfracbits);
}

static INLINE void
C16_MPYS_32(
    cint16 x,
    cint16 y,
    cint32 *acc,
    uint16 nfracbits
)
{
    acc->i -= HWROUND0(x.i * y.i - x.q * y.q, nfracbits);
    acc->q -= HWROUND0(x.i * y.q + x.q * y.i, nfracbits);
}

static INLINE cint32
C16C32_MPY(
    cint16 x,
    cint32 y,
    uint16 nfracbits
)
{
    cint32 z;

    z.i = HWROUND0( (x.i*y.i - x.q*y.q),nfracbits);
    z.q = HWROUND0( (x.i*y.q + x.q*y.i),nfracbits);
    return(z);
}


static INLINE cint32
C32C32_MPY(
    cint32 x,
    cint32 y,
    uint16 nfracbits
)
{
    cint32 z;

    z.i = HWROUND0( (x.i*y.i - x.q*y.q),nfracbits);
    z.q = HWROUND0( (x.i*y.q + x.q*y.i),nfracbits);
    return(z);
}

static INLINE void
C32_MPYA(
    cint32 x,
    cint32 y,
    cint32 *acc,
    uint16 nfracbits
)
{
    acc->i += HWROUND0(x.i * y.i - x.q * y.q, nfracbits);
    acc->q += HWROUND0(x.i * y.q + x.q * y.i, nfracbits);
}

static INLINE void
C32_MPYS(
    cint32 x,
    cint32 y,
    cint32 *acc,
    uint16 nfracbits
)
{
    acc->i -= HWROUND0(x.i * y.i - x.q * y.q, nfracbits);
    acc->q -= HWROUND0(x.i * y.q + x.q * y.i, nfracbits);
}

static INLINE cint64
C16C64_MPY(
    cint16 x,
    cint64 y,
    uint16 nfracbits
)
{
    cint64 z;

    z.i = HWROUND0( (x.i*y.i - x.q*y.q),nfracbits);
    z.q = HWROUND0( (x.i*y.q + x.q*y.i),nfracbits);
    return(z);
}

static INLINE cint64
C32C64_MPY(
    cint32 x,
    cint64 y,
    uint16 nfracbits
)
{
    cint64 z;

    z.i = HWROUND0( (x.i*y.i - x.q*y.q),nfracbits);
    z.q = HWROUND0( (x.i*y.q + x.q*y.i),nfracbits);
    return(z);
}


/***************************/
/*****    float add    *****/
/***************************/

static INLINE cfloat_t
CFLOAT_ADD(
    cfloat_t x,
    cfloat_t y
)
{
    cfloat_t z;

    z.i = x.i + y.i;
    z.q = x.q + y.q;
    return(z);
}

/***********************************/
/*****    float subtraction    *****/
/***********************************/
static INLINE cfloat_t
CFLOAT_SUB(
    cfloat_t x,
    cfloat_t y
)
{
    cfloat_t z;

    z.i = x.i - y.i;
    z.q = x.q - y.q;
    return(z);
}

/* float multiply */
static INLINE cfloat_t
CFLOAT_MPY(
    cfloat_t x,
    cfloat_t y
)
{
    cfloat_t z;

    z.i = x.i * y.i - x.q * y.q;
    z.q = x.i * y.q + x.q * y.i;
    return(z);
}

/* complext float conjugate */
static INLINE cfloat_t
CFLOAT_CONJ(
    cfloat_t x
)
{
    cfloat_t z;

    z.i = x.i;
    z.q = -x.q;
    return(z);
}

static INLINE cfloat_t
CFLOAT_NEG(
    cfloat_t x
)
{
    cfloat_t z;

    z.i = -x.i;
    z.q = -x.q;
    return(z);
}

static INLINE float_t
CFLOAT_NORM(
    cfloat_t x
)
{
    float_t z;

	z = x.i*x.i + x.q*x.q;
    return(z);
}


/* linear interpolator */
static INLINE float_t
INTERP1_LINEAR(
	float_t *ptr1,       /* ptr to lookup table input entries */
    float_t *ptr2,       /* ptr to lookup table output entries */
	int     sizeoftable, /* size of lookup table */
	float_t input        /* input value */
)
{
	float_t z = 0;
	int i;

	/* find the value in the input table closest to input */
	/* first check to see that input is as least as large as
	   the first table entry. if not, use the first entry */
	if ( input <= *ptr1 )
	{
		return(*ptr2);
	}
	else if ( input >= *(ptr1 + sizeoftable-1) )
	{
		/* input value is larger than largest table lookup entry.
		   just use the largest value then */
		return( *(ptr2 + sizeoftable - 1) );
	}
	else
	{
		/* input value is within the table, so now linearly interpolate */
        /* see if value is between values */
		i = 0;
		while ( i < (sizeoftable-1) )
		{
			if ( (input > *(ptr1+i)) && (input < *(ptr1+i+1)) )
			{
				/* we have a match, linearly interpolate between the values */
				z = *(ptr2+i) + (*(ptr2+i+1) - *(ptr2+i))/(*(ptr1+i+1) - *(ptr1+i))
					  * (input - *(ptr1+i) );
			}
			i++;
		}
		return(z);

	}

}


/*  ATANH operation */
static INLINE double
ATANH(
	double x
)
{
	double z;

	z = 0.5 * log( (1+x)/(1-x));
	return(z);
}


#endif /* _ARITH_UTILS_H_ */

