/*
 * percival_processing.h
 *
 *  Created on: 14 Jul 2015
 *      Author: pqm78245
 */

#ifndef INCLUDE_PERCIVAL_PROCESSING_H_
#define INCLUDE_PERCIVAL_PROCESSING_H_

#include "percival_exceptions.h"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>


template<typename T>
class percival_frame_mem{
public:
	/*
	 * typically width, height ~= 10,000, use signed short is sufficient. width * height ~= 100,000,000, 27 bit unsigned int, int is sufficient.
	 * if number of pixels is to be assumed less than range of int (INT_MIN - (2^31 -1)), width and height should be at most 2^15
	 * thus use int width, height to allow multiplications etc
	 * use a check in method set_frame_size to set width and height less than 2^15
	 *
	 * Evetually I still decide to use unsigned int for width and height, and unsigned int for all loop using width * height. This allows a larger range of widht and height.
	 * Width * height is guaranteed to be less than the range of unsigned int (0x7fffffff)
	 */
	unsigned int width, height;			//
	T* data;			/*128bits aligned*/
	T* not_aligned;
	//add an offset if splitting up the image is needed
	bool automatic_empty;

	void set_frame_size(unsigned int h, unsigned int w)
	{
		if((unsigned int)(h * w) > 0x7fffffff ){			//separated into two ifs to make code more readable
			throw datatype_exception("Image size overflows. Should be less than 32768 pixels in each dimension.");
		}else if(h < 0 || w < 0 || h * w < 0){
			throw datatype_exception("Image size overflows. Should be greater than or equal to 0 and less than 32768 pixels in each dimension.");
		}else{
			delete [] not_aligned;
			width = w;
			height = h;
			/*On current 64 bits machine, alignment defaults to 64 bits*/
			unsigned int align_to_N_bytes = 32;
			/*allocating memory*/
			not_aligned = new T[width * height + ( align_to_N_bytes + 32 )/sizeof(T)];	/*32Bytes extra space to align*/

			/*align to boundary*/
			std::size_t address = reinterpret_cast<std::size_t>(not_aligned);
			std::size_t offset = address % align_to_N_bytes;
			data = reinterpret_cast<T*>(reinterpret_cast<std::size_t>(not_aligned) + align_to_N_bytes - offset);
		}
	}

	percival_frame_mem(){not_aligned = new T[1]; set_frame_size(1,1); automatic_empty = true;}
	percival_frame_mem(unsigned int x, int y){not_aligned = new T[1]; set_frame_size(x,y); automatic_empty = true;}
	~percival_frame_mem(){
		if(automatic_empty){
			delete [] not_aligned;
			data = NULL;
			not_aligned = NULL;
		}
	}
};

template <typename T>
class percival_frame{
public:
	unsigned int width, height;
	T * data;
	std::vector<int> CDS_subtraction_indices;		//stores pixel indices requiring CDS_substraction

	percival_frame():
		width(1),
		height(1),
		data(NULL){}

	percival_frame(const percival_frame_mem<T> &obj):
		width(obj.width),
		height(obj.height),
		data(obj.data)
	{}

	percival_frame(const percival_frame &obj):
		width(obj.width),
		height(obj.height),
		data(obj.data)
	{}

	void set_frame_size(unsigned int h, unsigned int w)
	{
		if((unsigned int)(h * w) > 0x7fffffff ){			//separated into two ifs to make code more readable
			throw datatype_exception("Image size overflows. Should be less than 32768 pixels in each dimension.");
		}else if(h < 0 || w < 0 || h * w < 0){
			throw datatype_exception("Image size overflows. Should be greater than or equal to 0 and less than 32768 pixels in each dimension.");
		}else{
			width = w;
			height = h;
		}
	}

	void operator=(const percival_frame &obj){
		width = obj.width;
		height = obj.height;
		data = obj.data;
	}

	void operator=(const percival_frame_mem<T> &obj){
		width = obj.width;
		height = obj.height;
		data = obj.data;
	}
};


struct percival_calib_params{
public:
	static percival_frame<float> Gc;
	static percival_frame<float> Oc;
	static percival_frame<float> Gf;
	static percival_frame<float> Of;
	static percival_frame<float> ADU_to_electrons_conversion;
	static percival_frame<float> Gain_lookup_table1;
	static percival_frame<float> Gain_lookup_table2;
	static percival_frame<float> Gain_lookup_table3;
	static percival_frame<float> Gain_lookup_table4;
};

class percival_global_params{
private:
	static bool is_initialised;
	static bool is_initialised_every_member[255];

	//"KnifeQuadBPos1/";, rather than "KnifeQuadBPos1/!X!/Sample";
	static std::string top_level_data_set_name;

public:
	//frame and file properties
	static std::string default_path_name;
	static std::string default_data_set_name;			//make this private

	//calibration parameter properties
	static bool is_transposed_calib_params;
	static unsigned int calib_params_height;
	static unsigned int calib_params_width;

	static std::string default_location_Gc;
	static std::string default_location_Gf;
	static std::string default_location_Oc;
	static std::string default_location_Of;
	static std::string default_location_ADU_to_electrons_conversion;
	static std::string default_location_Gain_lookup_table1;
	static std::string default_location_Gain_lookup_table2;
	static std::string default_location_Gain_lookup_table3;
	static std::string default_location_Gain_lookup_table4;

	static std::string default_calib_params_dataset_name;

	percival_global_params();
	percival_global_params(const std::string & master_param_file);


	void initialisation(const std::string & master_param_file);
	bool initialised();
	bool load_master_param_file(const std::string & master_param_file);
	bool check_initialisation();
};


void percival_ADC_decode_correction_gain_multiplication(const percival_frame<unsigned short int> &, percival_frame<float> &, const percival_calib_params & calib_params, bool store_gain = false);
void percival_ADU_to_electron_correction(percival_frame<float> &CDS_Img, percival_frame<float> &output, const percival_calib_params &);
void percival_CDS_subtraction(percival_frame<float> &sample, const percival_frame<float> &reset, percival_frame<float>& output);

void percival_unit_gain_multiplication(const percival_frame<unsigned short int> & src_frame, const percival_frame<float> & calibrated, percival_frame<float> & output, const percival_calib_params & calib_params, bool check_dimensions = true);
void percival_unit_ADC_decode(const percival_frame<unsigned short int> &, percival_frame<unsigned short int> & Coarse, percival_frame<unsigned short int> & Fine, percival_frame<unsigned short int> & Gain);
void percival_unit_ADC_calibration(const percival_frame<unsigned short int> & Coarse,const  percival_frame<unsigned short int> & Fine, percival_frame<float>& output, const percival_calib_params &, bool check_dimensions = true);

template<typename T>
percival_frame<T> percival_align_memory(percival_frame_mem<T> & input, percival_frame_mem<T> & output_buffer, unsigned int stride, unsigned int boundary_size){
	/* Check size */
	unsigned int input_NoOfPixels = input.width * input.height;
	unsigned int output_NoOfPixels = output_buffer.width * output_buffer.height;
	if((input.width % stride))
		throw dataspace_exception("percival_align_memory: stride does not divide width.");

	if( ((input_NoOfPixels/stride))*boundary_size > output_NoOfPixels )
		throw dataspace_exception("percival_align_memory: spare space is insufficient.");

	if( reinterpret_cast<std::size_t>(input.data)% (sizeof(T) * boundary_size))
		throw dataspace_exception("percival_align_memory: first element of input data is not aligned.");
	percival_frame<T> output;
	output.height = input.height;
	output.width = (input.width / stride ) * boundary_size;

	for(unsigned int i = 0; i < input.width / stride; i ++){
		for (unsigned int j = 0; j < boundary_size; j ++){
			if(j < stride){
				*(output_buffer.data + i * boundary_size + j) = *(input.data + i * stride + j);
			}else{
				*(output_buffer.data + i * boundary_size + j) = 0;
			}
		}
	}

	output = output_buffer;
	return output;
}


#endif /* INCLUDE_PERCIVAL_PROCESSING_H_ */
