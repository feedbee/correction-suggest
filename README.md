correction-suggest
==================

C-written utility implements "Did you mead XXX?" mechanism.

Utility takes user entered words as arguments and prepare alternatives for each word in format
levenstein distance [tab] correction, one correction per string.

dict-build
----------
dict-build application gets to standart output words dictionary (one word per line)
and converts in to binary suggest-prepared format (puts in to standart output).

suggest
-------
Usage: suggest [-s short max_strlen_diff] [-l short max_levenstein_diff] [-p short parallel_proc_count] [-r short runs] [-d string dict_file] word | -h

