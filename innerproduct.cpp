#include <cilk/cilk.h>
#include <cilk/reducer_opadd.h>

#include <algorithm>
#include <numeric>
#include <string>
#include <iostream>
#include <cmath>
#include <iterator>
#include <functional>

#include "example_util_gettime.h"

#define COARSENESS 100000000
#define ITERS 10

double rec_cilkified(double * a, double * b, int n)
{
	if(n < COARSENESS){
		int i;
		double x = 0;
		for(i = 0; i < n; i++){
			x += a[i] * b[i];
			//std::cout << x << " = " << a[i] << " + " << b[i] << std::endl;
		}
		return x;
	}
	else{
		double x1 = cilk_spawn rec_cilkified(a,b,n/2);
		double x2 = rec_cilkified(&a[n/2],&b[n/2],n - n/2);
		cilk_sync;
		return x1 + x2;
	}
}

double loop_cilkified(double * a, double * b, int n)
{
	if(n < COARSENESS){
		return std::inner_product(a, a+n, b, (double)0);
	}
	int size = n / COARSENESS;
	if(n % COARSENESS > 0)
		size++;
	double x[size];
	cilk_for (int i = 0; i < size; i++){
		x[i] = 0;
		int offset = i * COARSENESS;
		int coarse = COARSENESS;
		if(n - offset < COARSENESS)
			coarse = n - offset;
		for(int j = 0; j < coarse; j++){
			x[i] += a[offset + j] * b[offset + j];
		}
	}
	double sum = 0;
	for(int k = 0; k < size; k++)
		sum += x[k];
	return sum;
}


double hyperobject_cilkified(double * a, double * b, int n)
{
	cilk::reducer_opadd<double> sum;
	cilk_for(int i = 0; i < n; i++)
		sum += a[i] * b[i];
        return sum.get_value();
}


int close(double x, double y, int n)
{
        double relative_diff = (x>y? (x-y)/x : (y-x)/x);
        return (relative_diff < sqrt((double) n) * exp2(-42))? 1 : 0;
}


// A simple test harness 
int inn_prod_driver(int n)
{
	double * a = new double[n];
	double * b = new double[n];
	for (int i = 0; i < n; ++i)
	{
        	a[i] = i;
		b[i] = i;
	}
    	std::random_shuffle(a, a + n);
	std::random_shuffle(b, b + n);
/*	for(int i=0; i < n; i++){
		std::cout << "a["  << i << "] = " << a[i] << ", b[" << i << "] = " << b[i] << std::endl;
	}*/
	double seqresult = std::inner_product(a, a+n, b, (double)0);	

	long t1 = example_get_time();
	for(int i=0; i< ITERS; ++i)
	{
		seqresult = std::inner_product(a, a+n, b, (double)0);	
	}
	long t2 = example_get_time();

	double seqtime = (t2-t1)/(ITERS*1000.f);
	std::cout << "Sequential time: " << seqtime << " seconds" << std::endl;	
	
	/***********************************************************/
	/********  START TESTING RECURSIVE CILKFIED VERSION  *******/
	/***********************************************************/

	double parresult = rec_cilkified(a, b, n);   
	t1 = example_get_time();
	for(int i=0; i< ITERS; ++i)
	{
		parresult = rec_cilkified(a, b, n);   
	}
 	t2 = example_get_time();

	double partime = (t2-t1)/(ITERS*1000.f);
	std::cout << "Recursive cilkified time:" << partime << " seconds" << std::endl;
	std::cout << "Speedup is: " << seqtime/partime << std::endl;
	std::cout << "Sequential result is: "<<seqresult<<std::endl;
	std::cout << "Recursive cilkified result is: "<<parresult<<std::endl;
	std::cout << "Result is " << (close(seqresult,parresult,n)  ? "correct":"incorrect") << std::endl; 
	
	/****************************************************************/
	/********  START TESTING NESTED LOOPED CILKIFIED VERSION  *******/
	/****************************************************************/
	parresult = loop_cilkified(a, b, n);   
	
	t1 = example_get_time();
	for(int i=0; i< ITERS; ++i)
	{
		//parresult = loop_cilkified(a, b, n);   
 	        parresult = loop_cilkified(a, b, n);   
	}
 	t2 = example_get_time();


	partime = (t2-t1)/(ITERS*1000.f);
	std::cout << "Nested loop cilkified time: " << partime << " seconds" << std::endl;
	std::cout << "Speedup is: " << seqtime/partime << std::endl;
	std::cout << "Sequential result is: "<<seqresult<<std::endl;
	std::cout << "Loop cilkified result is: "<<parresult<<std::endl;
	std::cout << "Result is " << (close(seqresult,parresult,n)  ? "correct":"incorrect") << std::endl; 
	
	/**************************************************************/
	/********  START TESTING HYPEROBJECT CILKIFIED VERSION  *******/
	/**************************************************************/

	parresult = hyperobject_cilkified(a, b, n);   
	
	t1 = example_get_time();
	for(int i=0; i< ITERS; ++i)
	{
		parresult = hyperobject_cilkified(a, b, n);   
	}
 	t2 = example_get_time();

	partime = (t2-t1)/(ITERS*1000.f);
	std::cout << "Hyperobject cilkified time:" << partime << " seconds" << std::endl;
	std::cout << "Speedup is: " << seqtime/partime << std::endl;
	std::cout << "Sequential result is: "<<seqresult<<std::endl;
	std::cout << "Hyperobject result is: "<<parresult<<std::endl;
	std::cout << "Result is " << (close(seqresult,parresult,n)  ? "correct":"incorrect") << std::endl; 
    	
        
	delete [] a;
	delete [] b;
    	return 0;
}

int main(int argc, char* argv[])
{
    int n = 1 * 1000 * 1000;
    if (argc > 1) {
        n = std::atoi(argv[1]);
    }
    std::cout << "Solving for vector size: " << n << std::endl;
    return inn_prod_driver(n);
}
