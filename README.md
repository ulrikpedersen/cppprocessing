[![Build Status](https://travis-ci.org/percival-detector/cppprocessing.svg)](https://travis-ci.org/percival-detector/cppprocessing)

cppProcessing
===================================================================

C++ implementation of the Percival processing algorithms

This project is a C++ implementation of pyProcessing algorithm library.

directories
-------------------------------------------------------------------
Basic algorithms
* data/                    data from test percival detector chip; data used for BOOST unit test
* Include/                 header files
* profile/                 main repeatedly calling algorithm functions, for profiling
* src/                     .cpp files
* test/unit_test/          BOOST unit tests files
* tools/                   generating HDF5 test data; python script for profiling. In separate README file.

Parallel algorithms
* parallel_test/           C++ main function as a driver for the library
* percival_parallel/       parallelized functions
 
AVX-based algorithm and function
* Include/percival_avx.h   Function object written in Intel Intrinsics. Algorithm ADC decode, correction, gain multiplication, CDS subtraction and Dark image subtraction.
* src/percival_avx.cpp	   Functionn wrapper for avx based function object.

List of functions
--------------------------------------------------------------------
Original Algorithms are,
```C++
percival_ADC_decode
percival_CDS_correction
percival_ADU_to_electron_correction
```
Parallelised Algorithms are,
```C++
unit_ADC_decode()
unit_ADC_calibration()
unit_ADC_gain_calibration()
```
Parallelised AVX algorithms are
```C++
percival_algorithm_avx_pf
```
HDF5-dependent functions are,
```C++
percival_HDF5_loader
percival_HDF5_writer
```
Dimension checking functions specific to some functions are,
```C++
percival_unit_ADC_decode_check
percival_unit_ADC_calibration_check
percival_unit_gain_multiplication_check
percival_CDS_correction_check
percival_ADU_to_electron_correction_check
```
Generic checks are,
```
percival_input_calib_dimension_check
percival_input_output_dimension_check
percival_input_calib_dimension_check_AVX
percival_input_calib_NULL_check
percival_null_check
```


Function objects encapsulate the actual algorithms to be run. They are used in both parallelised version and non-parallelised versions. In parallelised version, the ```range_iterator``` type is ```tbb::blocked_range <unsigned int>```, together with ```parallel_for``` templated function. In non-parallelised version, the ```range_iterator``` type is ```percival_range_iterator_mock_p```.

Extracting bits from the inital uint16 is coded in,
```C++
percival_unit_ADC_decode_p <range_iterator>
```
Linear scaling of the coarse and fine bits is coded in,
```
percival_unit_ADC_calibration_p <range_iterator>
```
Gain multiplication is coded in,
```
percival_unit_gain_multiplication_p <range_iterator>
```
A combined processing going from the start to the end of dark image subtraction is coded in,
```
percival_ADC_decode_pipe_p <range_iterator>
```
An AVX version of the previous algorithm is coded in,
```
percival_algorithm_avx <range_iterator>
```
Other objects are,
```C++
percival_frame <T>
percival_frame_mem <T>
percial_calib_params
percival_global_params
percival_range_iterator_mock_p
```
Where the difference between percival_frame and percival_frame_mem is that the latter can allocate and align memory.
The best performing function is ``` percival_algorithm_avx_pf ```.

AVX version was built in place of the original parallel version of the algorithm and there is currently no non-avx parallel version. It can be found in older versions though. Or, it can be built with ```percival_ADC_decode_pipe_p <range_iterator>```

How to run the test program
----------------------------
The ```main``` functions in parallel_test and Profile give two entry points to the library. Most of the measurements have been done using ```main``` in parallel_test. To run the function,

* step 1: create a set of calibration parameters using HDF5 files. These files can be created using one function in the python script. See the separate README.
* step 2: write a text file that contains the names and addresses of the calibration parameters generated previously, the format should be the same as the format in current file. Note the location of colons and quotation marks. This can also be generated by the python script.
* step 3: input commmand line parameters. They can be found in the comments of main function.
* step 4: modify a section of the main function code depending on whether AVX is used. Look for sign post in the comments.
* step 5: build the function and run.
* step 6: If the function runs, python script can be used to profile it or time it. Usually a frame size of 3717 by 3528 takes 30s to run 1000 times, including memory allocation time.
* step 7: If the function does not run, most likely the dimension of the calibration data is wrong. Regenerate the data. Other possible causes are in the choice of grain sizes which must be a factor of the number of pixels used.

Eclipse Build Configurations
----------------------------

The different products of this project are built using the Eclipse build system.
The following build configurations are available:

#### BOOST_test_ADC_decode

* **Description:**
	Build unit tests
	Require Boost Library and HDF5 C library (dynamic and static) to run
	HDF5 C library can be found in dls-sw
* **Build product:**
	BOOST_test_ADC_decode/cppProcessing2.0
* **Dependencies:**
	source: Include/, src/, tests/,
	test data files: data/
* **Files generated:**
	data/test_write_to_HDF5.hf

#### Profiling

* **Description:**
	Contains a main function. Used for profiling and testing.
	Require HDF5 C library (dynamic and static) to run
	HDF5 C library can be found in dls-sw
* **Build product:**
	Profiling/cppProcessing2.0
* **Dependencies:**
	source: Include/, src/, profile/,
	test data files: data/

#### test_data_generation

* **Description:**
	Generate test data for unit test on HDF5 loader
	Require HDF5 C++ library (dynamic and static) to run. C library will be used in later commits
* **Build product:**
	test_data_generation/cppProcessing2.0
* **Dependencies:**
	source: tools/HDF5_test_file_generator.cpp
* **Files generated:**
	data/test_HDF5.hf
	data/NotAHDF5File.txt

#### Debug

* **Description:**
	Currently unused build configuration
* **Build product:**


#### Release

* **Description:**
	Currently unused build configuration
* **Build product:**


Building with cmake
-------------------

For building the project without Eclipse, use the cmake configuration system.

Example:

    mkdir build
    cd build
    cmake ..
    make VERBOSE=1

The build should result in the following output:

* A library: pcvl-algorithms
* A binary executable: pcvl-profiling
* A unittest executable: pcvl-unittests

