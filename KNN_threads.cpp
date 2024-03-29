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
#include <pthread.h>

using namespace std;

struct Escopo{
	int k;
	int id;
	int n_threads;
	ArffData* train;
	ArffData* test;
	int* predictions;
};

float distance(ArffInstance* a, ArffInstance* b) {
    float sum = 0;
    
    // iterate through all of the attributes (except the last one, which is the class label)
    for (int i = 0; i < a->size()-1; i++) {
        float diff = (a->get(i)->operator float() - b->get(i)->operator float());
        sum += diff*diff;
    }
    
    return sum;
}

void *threadKNN(void *p)
{
    Escopo p_escopo = *(Escopo*) p;
	
    float* candidates = (float*) calloc(p_escopo.k*2, sizeof(float));
    for(int i = 0; i < 2*p_escopo.k; i++){ 
        candidates[i] = FLT_MAX; 
    }

    int num_classes = p_escopo.train->num_classes();
    int n_instances = p_escopo.test->num_instances();
    
    int* classCounts = (int*)calloc(num_classes, sizeof(int));

    int start = p_escopo.id * n_instances / p_escopo.n_threads;
    int end = (p_escopo.id+1) * n_instances / p_escopo.n_threads;
    if (p_escopo.id == p_escopo.n_threads - 1) end = n_instances;
    
    for(int queryIndex = start; queryIndex < end; queryIndex++) {
        for(int keyIndex = 0; keyIndex < p_escopo.train->num_instances(); keyIndex++) {
            float dist = distance(p_escopo.test->get_instance(queryIndex), p_escopo.train->get_instance(keyIndex));

            // Add to our candidates
            for(int c = 0; c < p_escopo.k; c++){
                if(dist < candidates[2*c]){
                    // Found a new candidate
                    // Shift previous candidates down by one
                    for(int x = p_escopo.k-2; x >= c; x--) {
                        candidates[2*x+2] = candidates[2*x];
                        candidates[2*x+3] = candidates[2*x+1];
                    }
                    
                    // Set key vector as potential k NN
                    candidates[2*c] = dist;
                    candidates[2*c+1] = p_escopo.train->get_instance(keyIndex)->get(p_escopo.train->num_attributes() - 1)->operator float();

                    break;
                }
            }
        }

        // Bincount the candidate labels and pick the most common
        for(int i = 0; i < p_escopo.k;i++){
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

        p_escopo.predictions[queryIndex] = max_index;
        
        for(int i = 0; i < 2*p_escopo.k; i++){ candidates[i] = FLT_MAX; }
        memset(classCounts, 0, num_classes * sizeof(int));
    }
    pthread_exit(NULL);
}


int* KNN(ArffData* train, ArffData* test, int k, int n_threads) {
    // Implements a sequential kNN where for each candidate query an in-place priority queue is maintained to identify the kNN's.
    
    pthread_t *threads;
    threads = (pthread_t*)malloc(n_threads * sizeof(pthread_t));
    //int* tids = (int*) malloc(n_threads * sizeof(int));
    
    int* predictions = (int*)malloc(test->num_instances() * sizeof(int));
    Escopo* p_escopo = (Escopo*)malloc(n_threads * sizeof(Escopo));
    
    for(int i = 0; i < n_threads; i++){
        p_escopo[i].k = k;
    	p_escopo[i].id = i;
    	p_escopo[i].n_threads = n_threads;
    	p_escopo[i].train = train;
    	p_escopo[i].test = test;
    	p_escopo[i].predictions = predictions;
    }
        
    for(int i = 0; i < n_threads; i++){
        pthread_create(&threads[i], NULL, threadKNN, (void*) &p_escopo[i]);
    }
      
    for(int i = 0; i < n_threads; i++){
        pthread_join(threads[i], NULL);  
    }
    
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

    if(argc != 5)
    {
        cout << "Usage: ./main datasets/trainfile.arff datasets/testfile.arff k n_threads" << endl;
        exit(0);
    }

    int k = strtol(argv[3], NULL, 10);
    int n_threads = strtol(argv[4], NULL, 10);

    // Open the datasets
    ArffParser parserTrain(argv[1]);
    ArffParser parserTest(argv[2]);
    ArffData *train = parserTrain.parse();
    ArffData *test = parserTest.parse();
    
    struct timespec start, end;
    int* predictions = NULL;
    
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    
    predictions = KNN(train, test, k, n_threads);
    
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);

    // Compute the confusion matrix
    int* confusionMatrix = computeConfusionMatrix(predictions, test);
    // Calculate the accuracy
    float accuracy = computeAccuracy(confusionMatrix, test);

    uint64_t diff = (1000000000L * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec) / 1e6;

    printf("P_THREADS :The %i-NN classifier for %lu test instances on %lu train instances required %llu ms CPU time. Accuracy was %.2f%%\n", k, test->num_instances(), train->num_instances(), (long long unsigned int) diff, (accuracy*100));
}
