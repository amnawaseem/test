/:=/ {
	print($0);
	if ($3 ~ /^[0-9]*$/) {
		print("$(info #define " $1 " " $3 ")");
	} else {
		print("$(info #define " $1 " \"" $3 "\")");
		if ($1 == "ARCH") {
			print("$(info #define ARCH_" toupper($3) ")");
		}
	}
	$0 = "";
}
/./ {
	print($0);
}
END {
	print("all:");
	print("	@:");
}
