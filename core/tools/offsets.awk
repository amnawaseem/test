BEGIN {
	STRUCT="";
	STRUCT_SIZEOF_LIST="";
}

# carry over include directives
/^#/ {
	print($0);
}

# we do not support nested struct declarations
/^struct / {
	if ($3 == "{") {
		if ((length(STRUCT) == 0) && (length(STRUCT_SIZEOF_LIST) == 0)) {
			print("const uint32_t __attribute__((section(\".rodata_off\"))) offset_table[] = {");
		}
		STRUCT=$2;
		gsub("_$", "", STRUCT);
		# print("// str_s " STRUCT);
		STRUCT_SIZEOF_LIST=STRUCT_SIZEOF_LIST "sizeof(" STRUCT "), "
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
		print("\toffset(" STRUCT ", " $0 "),");
	}
}

/^};/ {
	# print("// str_e " STRUCT);
	STRUCT="";
}

END {
	print("};");
	print("const uint32_t __attribute__((section(\".rodata_size\"))) sizeof_table[] = {");
	print("  " STRUCT_SIZEOF_LIST);
	print("};");
}
