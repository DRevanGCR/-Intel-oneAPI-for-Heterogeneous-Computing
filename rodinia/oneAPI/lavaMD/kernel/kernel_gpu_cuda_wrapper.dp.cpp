//========================================================================================================================================================================================================200
//	DEFINE/INCLUDE
//========================================================================================================================================================================================================200

//======================================================================================================================================================150
//	MAIN FUNCTION HEADER
//======================================================================================================================================================150

#include <CL/sycl.hpp>
#include <dpct/dpct.hpp>
#include "./../main.h" // (in the main program folder)	needed to recognized input parameters

//======================================================================================================================================================150
//	UTILITIES
//======================================================================================================================================================150

#include "./../util/device/device.h"				// (in library path specified to compiler)	needed by for device functions
#include "./../util/timer/timer.h"					// (in library path specified to compiler)	needed by timer

//======================================================================================================================================================150
//	KERNEL_GPU_CUDA_WRAPPER FUNCTION HEADER
//======================================================================================================================================================150

#include "./kernel_gpu_cuda_wrapper.h"				// (in the current directory)

//======================================================================================================================================================150
//	KERNEL
//======================================================================================================================================================150

#include "./kernel_gpu_cuda.dp.cpp" // (in the current directory)	GPU kernel, cannot include with header file because of complications with passing of constant memory variables

//========================================================================================================================================================================================================200
//	KERNEL_GPU_CUDA_WRAPPER FUNCTION
//========================================================================================================================================================================================================200

void 
kernel_gpu_cuda_wrapper(par_str par_cpu,
						dim_str dim_cpu,
						box_str* box_cpu,
						FOUR_VECTOR* rv_cpu,
						fp* qv_cpu,
						FOUR_VECTOR* fv_cpu)
{
 dpct::device_ext &dev_ct1 = dpct::get_current_device();
 sycl::queue &q_ct1 = dev_ct1.default_queue();

        //======================================================================================================================================================150
	//	CPU VARIABLES
	//======================================================================================================================================================150

	// timer
	long long time0;
	long long time1;
	long long time2;
	long long time3;
	long long time4;
	long long time5;
	long long time6;

	time0 = get_time();

	//======================================================================================================================================================150
	//	GPU SETUP
	//======================================================================================================================================================150

	//====================================================================================================100
	//	INITIAL DRIVER OVERHEAD
	//====================================================================================================100

        dpct::get_current_device().queues_wait_and_throw();

        //====================================================================================================100
	//	VARIABLES
	//====================================================================================================100

	box_str* d_box_gpu;
	FOUR_VECTOR* d_rv_gpu;
	fp* d_qv_gpu;
	FOUR_VECTOR* d_fv_gpu;

        sycl::range<3> threads(1, 1, 1);
        sycl::range<3> blocks(1, 1, 1);

        //====================================================================================================100
	//	EXECUTION PARAMETERS
	//====================================================================================================100

        blocks[2] = dim_cpu.number_boxes;
        blocks[1] = 1;
        threads[2] = NUMBER_THREADS; // define the number of threads in the block
        threads[1] = 1;

        time1 = get_time();

	//======================================================================================================================================================150
	//	GPU MEMORY				(MALLOC)
	//======================================================================================================================================================150

	//====================================================================================================100
	//	GPU MEMORY				(MALLOC) COPY IN
	//====================================================================================================100

	//==================================================50
	//	boxes
	//==================================================50

        d_box_gpu = (box_str *)sycl::malloc_device(dim_cpu.box_mem,
                                                   dpct::get_default_queue());

        //==================================================50
	//	rv
	//==================================================50

        d_rv_gpu = (FOUR_VECTOR *)sycl::malloc_device(
            dim_cpu.space_mem, dpct::get_default_queue());

        //==================================================50
	//	qv
	//==================================================50

        d_qv_gpu = (double *)sycl::malloc_device(dim_cpu.space_mem2,
                                                 dpct::get_default_queue());

        //====================================================================================================100
	//	GPU MEMORY				(MALLOC) COPY
	//====================================================================================================100

	//==================================================50
	//	fv
	//==================================================50

        d_fv_gpu = (FOUR_VECTOR *)sycl::malloc_device(
            dim_cpu.space_mem, dpct::get_default_queue());

        time2 = get_time();

	//======================================================================================================================================================150
	//	GPU MEMORY			COPY
	//======================================================================================================================================================150

	//====================================================================================================100
	//	GPU MEMORY				(MALLOC) COPY IN
	//====================================================================================================100

	//==================================================50
	//	boxes
	//==================================================50

        dpct::get_default_queue().memcpy(d_box_gpu, box_cpu, dim_cpu.box_mem).wait();

        //==================================================50
	//	rv
	//==================================================50

        dpct::get_default_queue().memcpy(d_rv_gpu, rv_cpu, dim_cpu.space_mem).wait();

        //==================================================50
	//	qv
	//==================================================50

        dpct::get_default_queue().memcpy(d_qv_gpu, qv_cpu, dim_cpu.space_mem2).wait();

        //====================================================================================================100
	//	GPU MEMORY				(MALLOC) COPY
	//====================================================================================================100

	//==================================================50
	//	fv
	//==================================================50

        dpct::get_default_queue().memcpy(d_fv_gpu, fv_cpu, dim_cpu.space_mem).wait();

        time3 = get_time();

	//======================================================================================================================================================150
	//	KERNEL
	//======================================================================================================================================================150

	// launch kernel - all boxes
        /*
        DPCT1049:3: The workgroup size passed to the SYCL kernel may exceed the
        limit. To get the device limit, query info::device::max_work_group_size.
        Adjust the workgroup size if needed.
        */
        dpct::get_default_queue().submit([&](sycl::handler &cgh) {
                sycl::accessor<FOUR_VECTOR, 1, sycl::access::mode::read_write,
                               sycl::access::target::local>
                    rA_shared_acc_ct1(sycl::range<1>(100), cgh);
                sycl::accessor<FOUR_VECTOR, 1, sycl::access::mode::read_write,
                               sycl::access::target::local>
                    rB_shared_acc_ct1(sycl::range<1>(100), cgh);
                sycl::accessor<double, 1, sycl::access::mode::read_write,
                               sycl::access::target::local>
                    qB_shared_acc_ct1(sycl::range<1>(100), cgh);

                cgh.parallel_for(sycl::nd_range<3>(blocks * threads, threads),
                                 [=](sycl::nd_item<3> item_ct1) {
                                         kernel_gpu_cuda(
                                             par_cpu, dim_cpu, d_box_gpu,
                                             d_rv_gpu, d_qv_gpu, d_fv_gpu,
                                             item_ct1,
                                             rA_shared_acc_ct1.get_pointer(),
                                             rB_shared_acc_ct1.get_pointer(),
                                             qB_shared_acc_ct1.get_pointer());
                                 });
        });

        checkCUDAError("Start");
        dpct::get_current_device().queues_wait_and_throw();

        time4 = get_time();

	//======================================================================================================================================================150
	//	GPU MEMORY			COPY (CONTD.)
	//======================================================================================================================================================150

        dpct::get_default_queue().memcpy(fv_cpu, d_fv_gpu, dim_cpu.space_mem).wait();

        time5 = get_time();

	//======================================================================================================================================================150
	//	GPU MEMORY DEALLOCATION
	//======================================================================================================================================================150

        sycl::free(d_rv_gpu, dpct::get_default_queue());
        sycl::free(d_qv_gpu, dpct::get_default_queue());
        sycl::free(d_fv_gpu, dpct::get_default_queue());
        sycl::free(d_box_gpu, dpct::get_default_queue());

        time6 = get_time();

	//======================================================================================================================================================150
	//	DISPLAY TIMING
	//======================================================================================================================================================150

	printf("Time spent in different stages of GPU_CUDA KERNEL:\n");

	printf("%15.12f s, %15.12f % : GPU: SET DEVICE / DRIVER INIT\n",	(float) (time1-time0) / 1000000, (float) (time1-time0) / (float) (time6-time0) * 100);
	printf("%15.12f s, %15.12f % : GPU MEM: ALO\n", 					(float) (time2-time1) / 1000000, (float) (time2-time1) / (float) (time6-time0) * 100);
	printf("%15.12f s, %15.12f % : GPU MEM: COPY IN\n",					(float) (time3-time2) / 1000000, (float) (time3-time2) / (float) (time6-time0) * 100);

	printf("%15.12f s, %15.12f % : GPU: KERNEL\n",						(float) (time4-time3) / 1000000, (float) (time4-time3) / (float) (time6-time0) * 100);

	printf("%15.12f s, %15.12f % : GPU MEM: COPY OUT\n",				(float) (time5-time4) / 1000000, (float) (time5-time4) / (float) (time6-time0) * 100);
	printf("%15.12f s, %15.12f % : GPU MEM: FRE\n", 					(float) (time6-time5) / 1000000, (float) (time6-time5) / (float) (time6-time0) * 100);

	printf("Total time:\n");
	printf("%.12f s\n", 												(float) (time6-time0) / 1000000);

}
