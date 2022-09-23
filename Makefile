todo: sequencial version1_0 version1_1 version2_0
sequencial: vs0_0_sequencial.cpp
	g++ -std=gnu++11 -o vs0_0_sequencial vs0_0_sequencial.cpp -Ilibarff libarff/arff_attr.cpp libarff/arff_data.cpp libarff/arff_instance.cpp libarff/arff_lexer.cpp libarff/arff_parser.cpp libarff/arff_scanner.cpp libarff/arff_token.cpp libarff/arff_utils.cpp libarff/arff_value.cpp
version1_0: vs1_0_OpenMP.cpp
	g++ -std=gnu++11 -fopenmp -o vs1_0_OpenMP vs1_0_OpenMP.cpp -Ilibarff libarff/arff_attr.cpp libarff/arff_data.cpp libarff/arff_instance.cpp libarff/arff_lexer.cpp libarff/arff_parser.cpp libarff/arff_scanner.cpp libarff/arff_token.cpp libarff/arff_utils.cpp libarff/arff_value.cpp
version1_1: vs1_1_threads.cpp
	g++ -std=gnu++11 -pthread -o vs1_1_threads vs1_1_threads.cpp -Ilibarff libarff/arff_attr.cpp libarff/arff_data.cpp libarff/arff_instance.cpp libarff/arff_lexer.cpp libarff/arff_parser.cpp libarff/arff_scanner.cpp libarff/arff_token.cpp libarff/arff_utils.cpp libarff/arff_value.cpp
version2_0: vs2_0_MPI.cpp
	mpicc -std=gnu++11 -o vs2_0_MPI vs2_0_MPI.cpp -lstdc++ -lmpi_cxx -Ilibarff libarff/arff_attr.cpp libarff/arff_data.cpp libarff/arff_instance.cpp libarff/arff_lexer.cpp libarff/arff_parser.cpp libarff/arff_scanner.cpp libarff/arff_token.cpp libarff/arff_utils.cpp libarff/arff_value.cpp
clean:
	rm vs0_0_sequencial vs1_0_OpenMP vs1_1_threads vs2_0_MPI
