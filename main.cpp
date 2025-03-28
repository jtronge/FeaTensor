#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <string.h>
#include "tensor.h"

int main(int argc, char *argv[])
{
	// To avoid outbut buffering
	setvbuf(stdout, NULL, _IONBF, 0);
	
	int opt;
	int method_choice = 0; //default : map
	char *filename = NULL;
	char *out_file_name = NULL;
	int is_binary = 0;
	int ask_csv = 1;
	int only3d = 0; // default : n-dim
	
	if (argc <= optind)
		printusage();

	while ((opt = getopt(argc, argv, "i:o:m:b:c:d:")) != -1) {
        switch (opt) {
        case 'i':
            filename = strdup(optarg);
            break;
        case 'o':
			out_file_name = optarg;
            break;
		case 'm':
			if(strncmp(optarg, "map", 3) == 0)
				method_choice = 0;
			else if(strncmp(optarg, "sort", 4) == 0)
				method_choice = 1;
			else if(strncmp(optarg, "group", 8) == 0)
				method_choice = 2;
			else if(strncmp(optarg, "hybrid", 6) == 0)
				method_choice = 3;
			else
				method_choice = 1;
			break;
		case 'b':
			is_binary = atoi (optarg);
            break;
		case 'c':
			ask_csv = atoi (optarg);
            break;
		case 'd':
			only3d = atoi (optarg);
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-t nsecs] [-n] name\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

	std::ifstream fin(filename);

	// Declare variables: (check the types here)
	TENSORSIZE_T nnz = 0;
	int order = 0;
	if (!fin.is_open())
	{
		std::cout << "File couldn't be opened." << std::endl;
		return 1;
	}

	while (fin.peek() == '#')
		fin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	std::string line;
	std::getline(fin, line);
	std::getline(fin, line);
	std::getline(fin, line);

	std::stringstream ss(line);
	std::string z;
	while (ss >> z)
		order++;
	if( !is_binary){
		order--;
	}

	while (fin.peek() != EOF)
	{
		if (fin.peek() != '#')
		{
			nnz++;
		}
		fin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	nnz++;
	
	timer *file_read_tm = timer_start("time_file_read");
	FILE *file_ptr;
	file_ptr = fopen(filename, "r");
	tensor *T;

	if (is_binary){
		T = read_tensor_binary(file_ptr, order, nnz);
	}
	else{
		T = read_tensor(file_ptr, order, nnz);
	}
	
	fclose(file_ptr);
	
	printf(" %s (tensor) || ", filename);
	printf(" %d (order) || ", order);
	for (int i = 0; i <order; i++){
		printf("\t %d", T->dim[i]);
	}
	printf(" (dim) || ");
	
	// printf("\nis_binary : \t%d \n", is_binary);
	printf(" %d (extraction_method) || ", method_choice);
	printf(" %d (max_threads) || ", omp_get_max_threads());
	
	if (method_choice != 0 && order > 3){	// Only map returns all features of M modes. Other methods return features of largest 3 modes. 
		only3d = 1;
	}
	
	T->org_dim = (int *)safe_malloc(order * sizeof(int));
	
	timer_end(file_read_tm);
	
	if (only3d){
	
		tensor *T_3D = create_tensor(3, nnz);
		T_3D->tensor_name = filename;
		T_3D->org_dim = (int *)safe_malloc(order * sizeof(int));
		
		tensor_to_3d (T, T_3D);
		
		timer *extract_feat_tm = timer_start("time_extract_features_total");
		mode_based_features *features = extract_features(T_3D, static_cast<EXTRACTION_METHOD>(method_choice));	
		timer_end(extract_feat_tm);

		std::ofstream out_file(out_file_name);

		if( ask_csv ){
			out_file << all_mode_features_to_csv(features, T_3D);
		}
		else{
			out_file << all_mode_features_to_json(features, T_3D);
		}
	}
	
	else{
		T->tensor_name = filename;
		T->org_order = T->order;

		timer *extract_feat_tm = timer_start("time_extract_features_total");
		mode_based_features *features = extract_features(T, static_cast<EXTRACTION_METHOD>(method_choice));	
		timer_end(extract_feat_tm);

		std::ofstream out_file(out_file_name);

		if( ask_csv ){
			out_file << all_mode_features_to_csv(features, T);
		}
		else{
			out_file << all_mode_features_to_json(features, T);
		}
	}
	
	printf(" \n");
	return 0;
}
