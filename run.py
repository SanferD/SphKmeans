from subprocess import Popen, PIPE
import sys

files = ['bag_of_words.csv', 'char3.csv', 'char5.csv', 'char7.csv']
cfile = 'reuters21578.class'
K = [20, 40, 60]
T = 20

def main():
	gen_ofn = lambda f,k: file.replace('.csv', '-'+str(k)+'.out')
	for file in files:
		for k in K:
			string = ['./sphkmeans', file, cfile, str(k), str(T), gen_ofn(file, k)]
			sys.stderr.write(' '.join(string) + '\n')
			
			process = Popen(string, stdout=PIPE)
			(output, err) = process.communicate()
			exit_code = process.wait()

			print(output)

if __name__=='__main__':
	main()
