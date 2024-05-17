#include <shared.h>

/**
 * @brief Applies the function ’f’ on each character of
the string passed as argument. And return 0 if the function ’f’ return 0
 *
 * @param s The string on which to iterate.
 * @param f The function to apply to each character.
 * @return 1 if all character pass the function ’f’
 */
int ft_stris(char *s, int (*f)(int))
{
	size_t i;
	size_t len;
	int result;

	i = 0;
	result = 1;
	if (s != NULL && f)
	{
		len = ft_strlen(s);
		while (i < len)
		{
			if ((*f)(s[i]) == 0)
				return (0);
			i++;
		}
	}
	return (result);
}
