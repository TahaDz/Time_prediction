import os


if __name__ == '__main__':

	i = 0
	a = list(range(8968,17000,8))#1000
	n_data = 1006
	for ni in a:
		if i < n_data:
					os.system("gcc Cfiles/jacobi1D.c -lOpenCL -lm -o Cfiles/jacobi1D -w -fcompare-debug-second")
					os.system("./Cfiles/jacobi1D 1 0 "+ str(ni))
					i += 1

	print(i)




