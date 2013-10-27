#include <dsp_lib/math-complex.h>

#ifdef __cplusplus
extern "C" {
#endif
/* Portable Windows DLL symbol export macro for DPI import tf's */
#define DPI_DLLESPEC
	DPI_DLLESPEC int64_t sv_time(void);
	DPI_DLLESPEC int32_t to_sv_complex16(int16_t r, int16_t i);
	DPI_DLLESPEC int64_t to_sv_complex32(int32_t r, int32_t i);
	DPI_DLLESPEC void from_sv_complex16(int32_t c, int16_t* r, int16_t* i);
	DPI_DLLESPEC void from_sv_complex32(int64_t c, int32_t* r, int32_t* i);

	DPI_DLLESPEC int32_t sin_lut(int16_t theta, int16_t lut_bits);
	DPI_DLLESPEC int32_t sin_lut12(int16_t theta, int16_t lut_bits);
	DPI_DLLESPEC int16_t expj_lut(int16_t theta, int16_t lut_bits, int16_t* c, int16_t* s);
	DPI_DLLESPEC int16_t expj_lut16(int16_t theta, int16_t* c, int16_t* s);

	DPI_DLLESPEC int16_complex expj(int16_t theta, int16_t lut_bits);
	DPI_DLLESPEC int16_complex expj16(int16_t theta);
	DPI_DLLESPEC int64_t complex_mult_with_expj16(int16_t theta, int16_complex in,
												  int16_t R, int16_t S);
	DPI_DLLESPEC int32_complex complex_mult_with_expj(int16_t theta, int16_t lut_bits,
													  int16_complex in,
													  int16_t R, int16_t S);

	DPI_DLLESPEC int hw_logsum(int ibits, int fbits, int fbits_output, int f, int g);
	DPI_DLLESPEC int hw_alog(int fbits, int x);
	DPI_DLLESPEC int hw_log(int fbits, int x);
	DPI_DLLESPEC double hw_logsumd(int ibits, int fbits, double f, double g);
	DPI_DLLESPEC double hw_alogd(int fbits, double x);
	DPI_DLLESPEC double hw_logd(int fbits, double x);
	DPI_DLLESPEC int32_t cordic_signs_calc(uint32_t iters,int32_t angle);
	DPI_DLLESPEC int32_t cordic_derotator(
										  uint32_t  trunc,
										  uint32_t  iters,
										  uint32_t  angle_bits,
										  int32_t in_i,
										  int32_t in_q,
										  int32_t* out_i,
										  int32_t* out_q,
										  uint32_t* signs,
										  int32_t *theta_obuf
									  );
	DPI_DLLESPEC int32_t cordic_derotator_handle_zero_in(
										  uint32_t  handle_zero,
										  uint32_t  trunc,
										  uint32_t  iters,
										  uint32_t  angle_bits,
										  int32_t in_i,
										  int32_t in_q,
										  int32_t* out_i,
										  int32_t* out_q,
										  uint32_t* signs,
										  int32_t *theta_obuf
									  );
	DPI_DLLESPEC int32_t cordic_rotator(
										uint32_t  trunc,
										uint32_t  iters,
										uint32_t  signs,
										int32_t in_i,
										int32_t in_q,
										int32_t* out_i,
										int32_t* out_q
									  );
	DPI_DLLESPEC int32_t auto_scale(
										 uint32_t trunc,
										 uint32_t bits,
										 uint32_t agc,
										 int32_t in_i,
										 int32_t in_q,
										 int32_t* out_i,
										 int32_t* out_q
										 );
	DPI_DLLESPEC int32_t recip_with_logs(
										 uint32_t bits,
										 int32_t in_i,
										 int32_t in_q
										 );
	DPI_DLLESPEC int32_t recip(
							   int32_t x,
							   int32_t s,
							   uint32_t iters
							   );
	DPI_DLLESPEC double recipd(
							   double x,
							   int iters
							   );
	DPI_DLLESPEC int32_t sqrt_recip(
							   int32_t x,
							   int32_t s,
							   uint32_t iters
							   );
	DPI_DLLESPEC double sqrt_recipd(
							   double x,
							   int iters
							   );

	DPI_DLLESPEC int32_t cordic_derotator_auto_scale(
									 uint32_t trunc,
									 uint32_t iters,
									 uint32_t angle_bits,
									 uint32_t scale_trunc,
									 uint32_t scale_bits,
									 uint32_t agc,
									 int32_t in_i,
									 int32_t in_q,
									 int32_t* out_i,
									 int32_t* out_q,
									 uint32_t* signs,
									 int32_t* theta_obuf
									 );
	DPI_DLLESPEC int32_t cordic_derotator_auto_scale_handle_zero_in(
									 uint32_t handle_zero,
									 uint32_t trunc,
									 uint32_t iters,
									 uint32_t angle_bits,
									 uint32_t scale_trunc,
									 uint32_t scale_bits,
									 uint32_t agc,
									 int32_t in_i,
									 int32_t in_q,
									 int32_t* out_i,
									 int32_t* out_q,
									 uint32_t* signs,
									 int32_t* theta_obuf
									 );
	DPI_DLLESPEC int32_t cordic_rotator_auto_scale(
													uint32_t trunc,
													uint32_t iters,
													uint32_t signs,
													uint32_t scale_trunc,
													uint32_t scale_bits,
													uint32_t agc,
													int32_t in_i,
													int32_t in_q,
													int32_t* out_i,
													int32_t* out_q
													);
	DPI_DLLESPEC int32_t loop_filter_run(
										 int32_t loop_filter_in,
										 int32_t alpha,
										 int32_t beta,
										 int en_mult,
										 int en_acc,
										 int reset,
										 int round_bits,
										 int32_t* loop_filter_out
										 );
	/* for FIFOs */
	DPI_DLLESPEC void* make_int_fifo(int sz);
	DPI_DLLESPEC int32_t get_fifo_data(void* fifo);
	DPI_DLLESPEC int put_fifo_data(void* fifo, int32_t data);
	/*DPI_DLLESPEC int fifo_write(void* fifo, int size, int32_t data);*/
	//	DPI_DLLESPEC void* make_sv_file(const char* filen);
	//DPI_DLLESPEC int get_file_status(void* buf);
	//DPI_DLLESPEC int get_file_data(void* buf);

#ifdef __cplusplus
}
#endif
