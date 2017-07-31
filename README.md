# SphKmeans


## How to run
To build, `make`.
Only tested on Linux.

First need to run `python parser.py` to ensure that the file is preprocessed correctly.
Then can run the algorithm on the input file. Takes roughly 15mins to run. I have already run this step so the files should be in the directory.

Preprocessing generates char3, char5, and char7 representations along with bag-of-words representation of the reuters21578 newspaper article dataset.
After preprocessing can run spherical k-means.

To run, `./sphkmeans <input-file> <class-file> <K> <max-iterations> <output-file>`
Takes about a minute to run.

Example usage `./sphkmeans bag_of_words.csv reuters21578.class 20 20 output.out`

## Breif Description
Spherical K-means is a clustering algorithm, i.e. an algorithm that tries to group data into K categories.
The data is a vector in N-dimensions and the algorithm essentially first picks K random vectors as centroid belonging to their own unique cluster, then assigns each of the N vectors to its closest centroid's cluster - where distance is measured using cosine similarity - next new centroids are found by averaging all the vectors in each cluster, and the process repeats until the clusters stop changing or max-iterations are exceeded.

