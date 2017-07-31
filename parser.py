# coding: utf-8

import math, os, fnmatch
from spec_chars import spec_chars

DEBUG = False 		# set to True to enable debug printing
REUTERS_DIR = 'reuters21578' 	# set to path of folder
all_articles = {}
word_counter = {}
reuters_tag_beg = '<REUTERS'
topics_tag_beg = "<TOPICS>"
topics_tag_end = "</TOPICS>"
text_tag_beg = '<TEXT>'
text_tag_end = '</TEXT>'

##
# prints to console only if DEBUG is true
def log(x):
	import sys

	if DEBUG:
		sys.stderr.write(x + '\n')

##
# parse the sgm file to get the articles information.
# updates the all_articles dictionary
# @param {string} filename		the fullpath to the sgm file
def parse_sgm_file(filename):
	f = open(filename, 'r')
	all_lines = f.readlines()
	all_lines.reverse()
	f.close()

	while len(all_lines) > 0:
		L = all_lines.pop()

		# check if the line contains the reuters tag
		if L[:len(reuters_tag_beg)] == reuters_tag_beg:

			# extract the NEWID
			pair = L[len(reuters_tag_beg):-1].split(' ').pop()
			NEWID = int( pair.split('=').pop()[1:-2] )

			# iterate through the tag's contents until the <TOPIC> tag is hit
			while not L[:len(topics_tag_beg)] == topics_tag_beg:
				L = all_lines.pop()

			# get all the topics contained inside the topics tag
			all_topics = L[len(topics_tag_beg):-len(topics_tag_end)]
			arr = all_topics.split("<D>")
			arr.pop(0)
			if len(arr) is 1:
				TOPIC = arr[0].split("</D>")[0]

				# iterate through the rest of the tag until <TEXT is encountered
				L = all_lines.pop()
				while L.find(text_tag_beg[:-1])==-1:
					L = all_lines.pop()

				# don't process if not normal <TEXT, i.e, if not <TEXT>
				if not L[:len(text_tag_beg)] == text_tag_beg:
					continue

				# iterate through all the <TEXT> contents until </TEXT> is encountered
				text = ""
				L = all_lines.pop()
				while not L[-len(text_tag_end)-1:-1] == text_tag_end:
					text += L
					L = all_lines.pop()

				# if L does not contain </BODY> then text contains </BODY>
				if L.find("</BODY>") == -1:
					text = text.split("</BODY>")[0]

				# since I am only extracting normal text tags the <BODY> tag is present
				BODY = text.split("<BODY>")[1]

				# add the body to the corresponding topic
				if not TOPIC in all_articles:
					all_articles[TOPIC] = []

				pair = (NEWID, BODY)
				all_articles[TOPIC].append(pair)



##
# get the most frequent topics
# @param {number}	how_many	number of frequent topics
# @return 				the array of 'how_many' most frequent topics
def get_frequent_topics(how_many):
	# fill mylist with each (# of articles, TOPIC) tuples
	mylist = []
	for TOPIC in all_articles:
		arr = all_articles[TOPIC]
		mylist.append( (len(arr), TOPIC) )

	# sort to get the most frequent topics
	mylist.sort( key = lambda t: t[0], reverse=True )
	sorted_list = mylist[:how_many]
	return map( lambda t: t[1], sorted_list )



##
# get the array of frequent articles. this array has the form
# (NEWID, TOPIC, ARTICLE). Note that each article is cleaned.
# @param {list<string>}	frequent_topics 	the list of frequent topics
# @return array of (NEWID, TOPIC, ARTICLE) where 	NEWID is a number
# 																								TOPIC is a string
# 																								ARTICLE is a list<string>
def get_frequent_articles(frequent_topics):
	frequent_articles = []
	for TOPIC in frequent_topics:
		articles = all_articles[TOPIC]
		for newid, article in articles:
			frequent_articles.append( (newid, TOPIC, article) )
	return frequent_articles



##
# cleans the body of an article to fit the assignment requirements
# @param	{string} 	   article   the article to clean
# @return	{list<string>}         an array of words of the cleaned article
def cleaner(article):
	# remove any words which begin with '&'
	for spec in spec_chars:
		article = article.replace(spec, "")

	# remove non-ascii characters: http://stackoverflow.com/a/20078869/4646773
	article = ''.join([i if ord(i) < 128 else '' for i in article])

	# change the character case to lower-case
	article = article.lower()

	# replace non-alphanumeric characters with a space
	article = ''.join([ i if (ord(i)>=48 and ord(i)<=57) or (ord(i)>=97 and ord(i)<=122) else ' ' for i in article ])

	# split the text into tokens using space as the delimeter
	words = article.split(' ')

	# eliminate any tokens that are empty or contain only digits or start with &
	words = filter(lambda x: x and not x.isdigit(), words)

	# find all unique words in article for word counting
	# remove any repeated words while maintaining the original order:
	# http://stackoverflow.com/a/480227/4646773
	seen = set()
	seen_add = seen.add
	unique_words = [x for x in words if not (x in seen or seen_add(x))]

	perform_word_counter(unique_words)
	return words



##
# increments the count for each word in the article
# @param {list<string>}	article 	the article used for updating the word count
def perform_word_counter(article):
	for word in article:
		if not word in word_counter:
			word_counter[word] = 0
		word_counter[word] += 1



##
# filters the words in article which do not occur at least threshold
# times in the article
# @param  {list<string>} article 	the article to filter
# @param  {number} 	 threshold		keep words with freq >= threshold
# @return {list<string>}					the filtered article
def filter_article(article, threshold):
	result = []
	for word in article:
		if word_counter[word] >= threshold:
			result.append(word)
	return result



##
# get all the words whose count is above a threshold
# @param {number} 	thresh 	the word count threshold to determine if it is frequent
# @return the list of frequent words sorted alphanumerically
def get_words(thresh):
	result = []
	for word in word_counter:
		if word_counter[word] >= thresh:
			result.append(word)

	result.sort()
	return result



##
# generate the bag of words representation for the document vector
# @param {list<string>} 	article 		the article from which to generate the frequency vector
# @param {list<string>} 	all_words 	the list of all frequent words which correspond to the dimensions of the document vector
# @return {list<number>} 							an array of word frequencies, not normalized
def bag_of_words(article, table, num_of_words):
	vector = [0]*num_of_words
	for word in article:
		vector[ table[word] ] += 1
	return vector



##
# returns the vector v after normalization
# @param {list<number>} v 	the vector to normalize
# @return {list<number>} 		the vector v after normalization
def normalize(v):
	vsqr = [el*el for el in v]
	vmag = math.sqrt( sum( vsqr ) )
	return [el/vmag for el in v]



##
# returns the input-file format of the provided vector
# id,dimension #,value only if value != 0.0
def generate_output_string(id, vec):
	out = ''

	# the rest of the output file
	for i, value in enumerate(vec):
		if float(value) > 0.0:
			out += str(id) + ',' + str(i) + ',' + str(value) + ','
	return out[:-1] + '\n'



##
# generate the bag of words representation of all the articles
def generate_bag_of_words(all_words, articles):
	# constant
	num_of_words = len(all_words)
	num_of_articles = len(articles)

	# generate a hashtable of each word with its index for fast lookup
	table = {}
	for i, word in enumerate(all_words):
		table[word] = i

	# generate bag of words and corresponding infile and classfile strings
	infile_string = ''
	for newid, topic, article in articles:
		vec = bag_of_words(article, table, num_of_words)
		infile_string += generate_output_string(newid, vec)

	# generate the lfile string
	label_string = ""
	for word in all_words:
		label_string += word + '\n'

	# save to file
	infile = open('bag_of_words.csv', 'w')
	lfile = open('bag_of_words.clabel', 'w')

	infile.write(infile_string)
	lfile.write(label_string)

	infile.close()
	lfile.close()



##
# generate the feature vector for the given input string
# @param {string} mystr 	the input string
# @param {number} n 			the sliding window size
# @return {list<string>} 	the list of n-grams of the given sliding window
def sliding_window(mystr, n):
	result = []
	for i in range(len(mystr)-n):
		result.append(mystr[i:i+n])
	return result



##
# generate the n-grams representation of each article
def generate_n_grams(articles, n):
	# the set of all n-grams
	all_grams = set()

	# generate all n-gram feature vectors
	all_vec = []
	for _id, topic, article in articles:
		fvec = sliding_window(' '.join(article), n)
		all_grams = all_grams | set(fvec)
		all_vec.append( (_id, topic, fvec) )

	# get the list of all_grams, sorted alpha-numerically
	all_grams = list(all_grams)
	all_grams.sort()
	log("number of " + str(n) + "-grams is " + str(len(all_grams)))

	# table for efficient index lookup
	table = {}
	for i, gram in enumerate(all_grams):
		table[gram] = i

	# get the sliding window vector and infile string
	num_of_grams = len(all_grams)
	num_of_articles = len(articles)
	infile_string = ''
	for newid, topic, fvec in all_vec:
		vector = bag_of_words(fvec, table, num_of_grams)
		infile_string += generate_output_string(newid, vector)

	# generate the lfile string
	label_string = ""
	for gram in all_grams:
		label_string += gram + '\n'

	# save to file
	infile = open('char'+str(n)+'.csv', 'w')
	lfile = open('char'+str(n)+'.clabel', 'w')

	infile.write(infile_string)
	lfile.write(label_string)

	infile.close()
	lfile.close()


def compute_stats(topics):
	lengths = [len(all_articles[topic]) for topic in topics]
	how_many = len(topics)

	total = sum(lengths)
	mean = total/how_many
	stddev = math.sqrt(  sum([ (mean-length)**2 for length in lengths ])/how_many  )
	minimum = min(lengths)
	maximum = max(lengths)

	log('total number of documents is ' + str(total))
	log('avg cluster size is ' + str(mean))
	log('avg stddev size is ' + str(stddev))
	log('minimum cluster size is ' + str(minimum))
	log('maximum cluster size is ' + str(maximum))


def main():
	# get the list of all *.sgm files in REUTERS_DIR
	files = fnmatch.filter(os.listdir(REUTERS_DIR), '*.sgm')
	files.sort()
	files = map( lambda x: os.path.join(REUTERS_DIR, x), files )

	# parse each file
	for file in files:
		log(file)
		parse_sgm_file(file)

	# get all articles belonging to top 20 frequent topics
	how_many = 20
	frequent_topics = get_frequent_topics(how_many)
	frequent_articles = get_frequent_articles(frequent_topics)

	# get some article statistics
	if DEBUG:
		compute_stats(frequent_topics)

	#
	all_articles.clear()

	# clean the articles
	cleaned_articles = map(lambda (_1, _2, _3): (_1, _2, cleaner(_3)), frequent_articles)

	# filter each article based on wordcount so only keep words with wordcout>=5
	thresh = 5
	articles = map( lambda (_1, _2, _3): (_1, _2, filter_article(_3, thresh)), cleaned_articles )

	# make sure there are no empty articles
	articles = filter( lambda (_1, _2, _3): len(_3)>0 , articles)

	# get all words above threshold
	all_words = get_words(thresh)

	# generate the class file
	classes = map( lambda (x,y,z): (x,y), articles )
	classes.sort(key=lambda (x,y): x)
	classes = map( lambda (_1,_2): str(_1)+','+str(_2), classes )

	lfile = open('reuters21578.class', 'w')
	lfile.write('\n'.join(classes) + '\n')
	lfile.close()

	# generate the bag of words representation
	articles.sort()
	log("number of words is " + str( len(all_words) ) )
	generate_bag_of_words(all_words, articles)
	all_words[:] = [] # free up ram

	# generate the n-gram representation of all the artices
	for n in [3, 5, 7]:
		generate_n_grams(articles, n)

if __name__ == "__main__":
	main()
