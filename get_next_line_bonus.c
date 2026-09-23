/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line_bonus.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vladyslb <vladyslb@student.42prague.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/23 10:00:00 by vladyslb          #+#    #+#             */
/*   Updated: 2026/09/23 10:00:00 by vladyslb         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "get_next_line_bonus.h"

static char	*fill_stash(char **stash, int fd)
{
	ssize_t	bytes;
	char	*temp;
	char	*buf;

	buf = malloc(BUFFER_SIZE + 1);
	if (!buf)
		return (NULL);
	bytes = read(fd, buf, BUFFER_SIZE);
	if (bytes <= 0)
	{
		free(buf);
		if (bytes < 0)
		{
			free(*stash);
			*stash = NULL;
		}
		return (NULL);
	}
	buf[bytes] = '\0';
	temp = ft_strjoin(*stash, buf);
	free(buf);
	free(*stash);
	*stash = temp;
	return (*stash);
}

static char	*fill_line(char *stash)
{
	size_t	len;
	size_t	i;
	char	*line;

	len = 0;
	while (stash[len] && stash[len] != '\n')
		len++;
	if (stash[len] == '\n')
		len++;
	line = malloc(len + 1);
	if (!line)
		return (NULL);
	i = 0;
	while (i < len)
	{
		line[i] = stash[i];
		i++;
	}
	line[i] = '\0';
	return (line);
}

static char	*stash_cut(char *stash)
{
	size_t	i;
	size_t	len;
	char	*new;

	i = 0;
	while (stash[i] && stash[i] != '\n')
		i++;
	if (stash[i] == '\n')
		i++;
	len = 0;
	while (stash[i + len])
		len++;
	new = malloc(len + 1);
	if (!new)
		return (NULL);
	len = 0;
	while (stash[i + len])
	{
		new[len] = stash[i + len];
		len++;
	}
	new[len] = '\0';
	return (new);
}

char	*free_stash(char **stash)
{
	free(*stash);
	*stash = NULL;
	return (NULL);
}

char	*get_next_line(int fd)
{
	static char	*stash[FD_SETSIZE];
	char		*temp;
	char		*line;

	if (fd < 0 || fd >= FD_SETSIZE || BUFFER_SIZE <= 0)
		return (NULL);
	if (!stash[fd])
		stash[fd] = ft_strdup("");
	if (!stash[fd])
		return (NULL);
	while ((!ft_strchr(stash[fd], '\n')) && fill_stash(&stash[fd], fd))
		;
	if (!stash[fd])
		return (NULL);
	if (!*stash[fd])
		return (free_stash(&stash[fd]));
	line = fill_line(stash[fd]);
	if (!line)
		return (free_stash(&stash[fd]));
	temp = stash_cut(stash[fd]);
	free(stash[fd]);
	stash[fd] = temp;
	return (line);
}
