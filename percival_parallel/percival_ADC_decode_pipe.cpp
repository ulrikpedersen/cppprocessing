/*
 * percival_ADC_decode_pipe.cpp
 *
 *  Created on: 20 Aug 2015
 *      Author: pqm78245
 */


#include "percival_processing.h"
#include "percival_parallel.h"
#include "tbb/pipeline.h"

void percival_ADC_decode_pipe(
		const percival_frame<unsigned short int> & input,
		percival_frame<float> & output,
		const percival_calib_params & calib_params,
		percival_frame<unsigned short int> gain,
		percival_frame<unsigned short int> fine,
		percival_frame<unsigned short int> coarse,
		percival_frame<float> calibrated,
		bool store_gain)
{
	unsigned int height = input.height;
	unsigned int width = input.width;

	//	percival_frame<unsigned short int> gain(height, width);
	//	percival_frame<unsigned short int> fine(height, width);
	//    percival_frame<unsigned short int> coarse(height, width);
	//	percival_frame<float> calibrated(height, width);
	unsigned int NoOfPixels = output.width * output.height;

	percival_ADC_decode_pipe_p< tbb::blocked_range<unsigned int> > percival_ADC_decode_pipe_p (input , output, calib_params, gain, fine, coarse, calibrated);
	tbb::parallel_for( tbb::blocked_range<unsigned int>(0, NoOfPixels), percival_ADC_decode_pipe_p, tbb::auto_partitioner());

}


void percival_ADC_decode_pf(
		const percival_frame<unsigned short int> & src_frame,
		percival_frame<float> & des_frame,
		const percival_calib_params & calib_params,
		unsigned int grain_size,
		bool store_gain)
{
	//initialize destination matrix
	if(src_frame.width != des_frame.width || src_frame.height != des_frame.height){
		throw dataspace_exception("percival_ADC_decode: output and input dimension mismatch.");
	}//Saving time for memory allocation

	if(calib_params.Gc.height != src_frame.height){
		throw dataspace_exception("percival_ADC_decode: calibration array height and sample array height mismatch.");
	}

	if(calib_params.Gc.width != 7)
		throw dataspace_exception("percival_ADC_decode: calibration array width and sample array width mismatch. Expected: width == 7.");

	unsigned int NoOfPixels = src_frame.width * src_frame.height;

	percival_ADC_decode_p< tbb::blocked_range<unsigned int> > ADC_decode_p (src_frame, des_frame, calib_params);
	tbb::parallel_for( tbb::blocked_range<unsigned int>(0, NoOfPixels, 1000), ADC_decode_p);

	//	percival_ADC_decode_p< percival_range_iterator_mock_p > ADC_decode_p (src_frame, des_frame, calib_params);
	//	percival_range_iterator_mock_p range(0, NoOfPixels);
	//	unsigned int NoOfGrains = NoOfPixels / grain_size;
	//	unsigned int lower, upper;
	//	for(unsigned int i = 0; i < NoOfGrains; ++i){
	//		lower = i * grain_size;
	//		upper = (i + 1) * grain_size;
	//		range.lower = lower;
	//		range.upper = upper;
	//		ADC_decode_p(range);
	//	}


}

/*===============================================================================================================================*/
/* Pipeline design mark four*/
/* Ultimate gigantic function */
/*
 *  Generate a stream of data for later stages
 */
/*
 *
 *	A few rows as a token.
 *	the number of rows should be a factor of 3717 = ( 3 * 3 * 7 * 59)
 *  Possible choices are
 *  	1,3,7,9,21, 59,63,177,413,531,
 *  	1239,3717
 *
 *	Or use a fraction of a row as tokens.
 *	The fraction should be a factor of 3528 = (2 * 2 * 2 * 3 * 3 * 7 * 7 * 9)
 *	Possible choices are :
 *		1, 2, 3, 4, 6,  7, 8, 9, 12, 14,
 *		18, 21, 24, 28, 36,  42, 49,  56, 63, 72,
 *		84, 98, 126, 147, 168,  196, 252, 392, 441,504,
 *		588, 882, 1176, 1764, 3528
 */

/* Generating a stream of data*/
/* For segmentation */

class percival_pipeline_stream_generator : public tbb::filter{
private:

	unsigned int offset;	/*offset within the frame*/
	unsigned int* initial_ptr;
	const unsigned int grain_size;
	const unsigned int size;	/* image size to issue an end of frame */
	const unsigned int max_number_of_tokens;
	unsigned int current_index;
	unsigned int next_index;

public:
	percival_pipeline_stream_generator(
			unsigned int* initial_ptr,
			unsigned int grain_size,
			unsigned int frame_size,
			unsigned int max_number_of_tokens
	):
		tbb::filter(/*is_serial=*/true),
		initial_ptr( initial_ptr ),
		grain_size( grain_size ),
		offset( 0 ),
		size( frame_size ),
		max_number_of_tokens(max_number_of_tokens),
		current_index( 0 ),
		next_index( 0 )
	{}

	void* operator()(void*){
		if(offset < size){
			current_index = next_index;
			*(initial_ptr + current_index) = offset;
			next_index++;
			if(next_index == max_number_of_tokens){
				next_index = 0;
			}
			offset += grain_size;
			return initial_ptr + current_index;
		}else{
			return NULL;
		}
	}
};


void percival_ADC_decode_combined_pipeline(
		const percival_frame<unsigned short int> & sample,
		const percival_frame<unsigned short int> & reset,
		percival_frame<float> & output,
		const percival_calib_params & calib_params,
		unsigned int grain_size,
		bool store_gain)
{
	/* Maximum number of tokens in existence at one point in time */
	unsigned int max_tokens = 20;

	/* starting a pipeline */
	tbb::pipeline pipeline;

	/*
	 * A list of offset from the start of image array,
	 * corresponding to each token
	 *
	 * */
	unsigned int offset_arr [max_tokens];
	unsigned int *offset_ptr = & offset_arr[0];

	/* Initialising the input image struct */
	CDS_output CDS_input;
	CDS_input.input_sample = sample;
	CDS_input.input_reset = reset;
	CDS_input.output = output;

	/*
	 *  Constructing the pipeline
	 *
	 */

	percival_pipeline_stream_generator Input(offset_ptr, grain_size, sample.height * sample.width, max_tokens);
	pipeline.add_filter( Input );

	ADC_decode_filter4<CDS_output> ADC_decode_CDS ( CDS_input, calib_params, grain_size );
	pipeline.add_filter( ADC_decode_CDS );

	pipeline.run( max_tokens );

	pipeline.clear();
}

