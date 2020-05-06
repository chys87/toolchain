#! /usr/bin/python3
# -*- coding: utf-8 -*-

import re


def apply_re(contents, lst):
    """Apply full-text regular expression replace to contents.

    lst = [(pattern, sub), (pattern, sub), ...]
    pattern can either be bytes/string or a re object.
    """
    for (pattern, sub) in lst:
        if isinstance(pattern, (bytes, str)):
            contents = re.sub(pattern, sub, contents, flags=re.M | re.A)
        else:
            contents = re.sub(pattern, sub, contents)
    return contents


match_type = type(re.match('x', 'x'))
