#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <float.h>
#include <math.h>
#include <tuple>
#include <iostream>
#include "libarff/arff_parser.h"
#include "libarff/arff_data.h"
#include <bits/stdc++.h>
#include "mpi.h"

using namespace std;

float distance(ArffInstance* a, ArffInstance* b) {
    float sum = 0;
    
    // iterate through all of the attributes (except the last one, which is the class label)
    for (int i = 0; i < a->size()-1; i++) {
        float diff = (a->get(i)->operator float() - b->get(i)->operator float());
        sum += diff*diff;
    }
    
    return sum;
}

// Implements a sequential kNN where for each candidate query an in-place priority queue is maintained to identify the kNN's.
int* KNN(ArffData* train, ArffData* test, int k, int rank, int size) {
    
    int num_instances = test->num_instances();
    int num_virtual = (num_instances % size) + num_instances;
    int num_local = num_virtual/size;
    int num_classes = train->num_classes();
    
    float* candidates = (float*) calloc(k*2, sizeof(float));
    int* classCounts = (int*)calloc(num_classes, sizeof(int));
    int* predictions = (int*)malloc(num_virtual * sizeof(int));
           
    for(int i = 0; i < 2*k; i++){ 
        candidates[i] = FLT_MAX; 
    }

    int start = rank * num_local;
    int end = start + num_local;
    if (rank == size -1){
    	end = num_instances;
    }

    int* local_predictions = (int*)malloc(num_local * sizeof(int));
    
    int j = 0;

    for(int queryIndex = start; queryIndex < end; queryIndex++) {
        for(int keyIndex = 0; keyIndex < train->num_instances(); keyIndex++) {
            float dist = distance(test->get_instance(queryIndex), train->get_instance(keyIndex));

            for(int c = 0; c < k; c++){
                if(dist < candidates[2*c]){
                    for(int x = k-2; x >= c; x--) {
                        candidates[2*x+2] = candidates[2*x];
                        candidates[2*x+3] = candidates[2*x+1];
                    }
                    candidates[2*c] = dist;
                    candidates[2*c+1] = train->get_instance(keyIndex)->get(train->num_attributes() - 1)->operator float(); // class value
                    break;
                }
            }
        }

        for(int i = 0; i < k;i++){
            classCounts[(int)candidates[2*i+1]] += 1;
        }
        
        int max = -1;
        int max_index = 0;
        for(int i = 0; i < num_classes;i++){
            if(classCounts[i] > max){
                max = classCounts[i];
                max_index = i;
            }
        }

        local_predictions[j] = max_index;
        j++;

        for(int i = 0; i < 2*k; i++){ candidates[i] = FLT_MAX; }
        memset(classCounts, 0, num_classes * sizeof(int));
    }
    
    MPI_Gather (local_predictions, num_local, MPI_INT, predictions, num_local, MPI_INT, 0, MPI_COMM_WORLD);

    return predictions;
}

int* computeConfusionMatrix(int* predictions, ArffData* dataset)
{
    int* confusionMatrix = (int*)calloc(dataset->num_classes() * dataset->num_classes(), sizeof(int)); // matrix size numberClasses x numberClasses
    
    for(int i = 0; i < dataset->num_instances(); i++) // for each instance compare the true class and predicted class
    {
        int trueClass = dataset->get_instance(i)->get(dataset->num_attributes() - 1)->operator int32();
        int predictedClass = predictions[i];
        
        confusionMatrix[trueClass*dataset->num_classes() + predictedClass]++;
    }
    
    return confusionMatrix;
}

float computeAccuracy(int* confusionMatrix, ArffData* dataset)
{
    int successfulPredictions = 0;
    
    for(int i = 0; i < dataset->num_classes(); i++)
    {
        successfulPredictions += confusionMatrix[i*dataset->num_classes() + i]; // elements in the diagonal are correct predictions
    }
    
    return successfulPredictions / (float) dataset->num_instances();
}

int main(int argc, char *argv[]){
    
    if(argc != 4)
    {
        cout << "Usage: n_process main datasets/trainfile.arff datasets/testfile.arff k" << endl;
        exit(0);
    }

    int k = strtol(argv[3], NULL, 10);
    
    int rank, size;
    MPI_Init(&argc,&argv);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
    MPI_Comm_size (MPI_COMM_WORLD, &size);

    // Open the datasets
    ArffParser parserTrain(argv[1]);
    ArffParser parserTest(argv[2]);
    ArffData *train = parserTrain.parse();
    ArffData *test = parserTest.parse();
    
    struct timespec start, end;
    int* predictions = NULL;
    
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    
    predictions = KNN(train, test, k, rank, size);
    
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);

    if (rank == 0) 
    {
    	// Compute the confusion matrix
    	int* confusionMatrix = computeConfusionMatrix(predictions, test);
    	// Calculate the accuracy
    	float accuracy = computeAccuracy(confusionMatrix, test);

    	uint64_t diff = (1000000000L * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec) / 1e6;

    	printf("The %i-NN classifier for %lu test instances on %lu train instances required %llu ms CPU time. Accuracy was %.2f%%\n", k, test->num_instances(), train->num_instances(), (long long unsigned int) diff, (accuracy*100));
    }
    MPI_Finalize();
}
