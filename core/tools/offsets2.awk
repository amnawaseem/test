BEGIN {
	OFFSET_VALUES="";
	SIZEOF_VALUES="";
	MODE=0;
	OFFSETVAR_INDEX=1;
	print("# define\toffset(structname, membername)\toffset__##structname##__##membername")
	print("# define\tsizeof(structname)\t\tsizeof__##structname")
	print("# define\tlogsizeof(structname)\t\tlogsizeof__##structname")
}

BEGINFILE {
	MODE+=1;
	if (MODE == 3) {
		split(OFFSET_VALUES, OFFSET_VALARRAY);
		split(SIZEOF_VALUES, SIZEOF_VALARRAY);
	}
}

/^0x/ {
	if (MODE == 1) {
		OFFSET_VALUES=OFFSET_VALUES " " $1;
	}
	if (MODE == 2) {
		SIZEOF_VALUES=SIZEOF_VALUES " " $1;
	}
}

# we do not support nested struct declarations
/^struct / {
	if ($3 == "{") {
		STRUCT=$2;
		gsub("_$", "", STRUCT);
		# print("// str_s " STRUCT);
		STRUCT_SIZEOF_LIST=STRUCT_SIZEOF_LIST " " STRUCT;
	}
}

# create an offset() value for every member
# chop off:
#   array specifications [...]
#   final semicolon
#   pointer levels
/^[[:space:]].*;$/ {
	if (length(STRUCT) > 0) {
		gsub(";$", "");
		gsub("[[][^]]*[]]$", "");
		gsub("^.*[[:space:]]", "");
		gsub("^[*]*", "");
		print("#define\toffset__" STRUCT "__" $0 "\t" OFFSET_VALARRAY[OFFSETVAR_INDEX])
		OFFSETVAR_INDEX+=1;
	}
}

/^};/ {
	# print("// str_e " STRUCT);
	STRUCT="";
}

END {
	STRUCT_COUNT=split(STRUCT_SIZEOF_LIST, STRUCT_ARRAY);
	for (i=1; i<=STRUCT_COUNT; i+=1) {
		print("#define\tsizeof__" STRUCT_ARRAY[i] "\t" SIZEOF_VALARRAY[i])
		for (j=2; j<32; j+=1) {
			k=sprintf("0x%x", 2**j)
			if (SIZEOF_VALARRAY[i] == k) {
				print("#define\tlogsizeof__" STRUCT_ARRAY[i] "\t" j)
			}
		}
	}
}
