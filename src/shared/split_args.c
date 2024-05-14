#include <stdlib.h>
#include <string.h>

typedef enum quote_e
{
	nil,
	doubleq,
	singleq
} quote_t;

static int get_words_count(const char *str)
{
	if (str == NULL)
		return 0;
	int count = 0;
	quote_t in_quote = nil;
	int i = 0;
	while (str[i] == ' ')
		i++;
	while (str[i])
	{
		if (str[i] == '\'' && in_quote == nil)
			in_quote ^= singleq;
		else if (str[i] == '"' && in_quote == nil)
			in_quote ^= doubleq;
		else if (in_quote == nil && str[i] == ' ')
		{
			count++;
			while (str[i] == ' ')
				i++;
		}
		i++;
	}
	return count + 1;
}

typedef struct
{
	char *str;
	int len;
	int index;
} word_t;

static word_t assign_str(const char *str)
{
	int len = 0;
	quote_t in_quote = nil;
	int i = 0;

	// Skip spaces
	while (str[i] == ' ')
		i++;
	// Count the length of the word without quotes
	while (str[i])
	{
		if (str[i] == '\'' && in_quote != doubleq)
			in_quote ^= singleq;
		else if (str[i] == '"' && in_quote != singleq)
			in_quote ^= doubleq;
		else if (in_quote == nil && str[i] == ' ')
			break;
		else
			len++;
		i++;
	}

	char *word = calloc(len + 1, sizeof(char));
	if (word == NULL)
		return (word_t){NULL, 0, 0};
	len = 0;
	i = 0;
	while (str[i] == ' ')
		i++;
	while (str[i])
	{
		if (str[i] == '\'' && in_quote != doubleq)
			in_quote ^= singleq;
		else if (str[i] == '"' && in_quote != singleq)
			in_quote ^= doubleq;
		else if (in_quote == nil && str[i] == ' ')
			break;
		else
		{
			word[len] = str[i];
			len++;
		}
		i++;
	}
	return (word_t){word, len, i};
}

/**
 *  @brief Split a string into an array of strings by spaces
 *  If a string is between single or double quotes, it will be considered as one string
 *
 *  @example split_space("Hello World") // => ["Hello", "World"]
 *  @example split_space("'Hello World'") // => ["Hello World"]
 * @param to_split The string to split
 * @retval char** An array of allocated strings
 * @retval NULL If an allocation failed
 */
char **split_space(const char *to_split)
{
	if (to_split == NULL)
		return NULL;

	int total_av = get_words_count(to_split);

	char **av = calloc(total_av + 1, sizeof(char *));
	if (av == NULL)
		return NULL;

	int i = 0;
	while (to_split[0])
	{
		word_t word = assign_str(to_split);
		if (word.str == NULL)
		{
			for (int j = 0; av[j]; j++)
				free(av[j]);
			free(av);
			return NULL;
		}
		av[i] = word.str;
		i++;
		to_split += word.index;
		while (*to_split == ' ')
			to_split++;
	}
	return av;
}
