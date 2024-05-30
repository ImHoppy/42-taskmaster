#include "taskmasterd.h"
#include <assert.h>

process_vec_t *init_process_vec(int size)
{
	process_vec_t *vec = calloc(sizeof(process_vec_t), 1);
	assert(vec != NULL);
	vec->data = calloc(sizeof(process_t), size);
	assert(vec->data != NULL);

	vec->size = size;
	return vec;
}

bool push_process_vec(process_vec_t *vec, process_t *process)
{
	if (vec->index >= vec->size)
	{
		vec->data = realloc(vec->data, vec->size + 32);
		assert(vec->data != NULL);
	}
	vec->data[vec->index] = process;
	vec->index++;

	return true;
}

bool free_process_vec(process_vec_t *vec)
{
	free(vec->data);
	free(vec);
}
