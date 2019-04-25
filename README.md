# DegreesOfSeparation
Calculate degrees of separation for any two actors on IMDb dataset (17.3 million records)

From Wikipedia, the free encyclopedia

The Bacon number of an actor or actress is the number of degrees of separation (see Six degrees of separation) they have from actor Kevin Bacon, as defined by the game known as Six Degrees of Kevin Bacon. It applies the Erdős number concept to the movie industry. The higher the Bacon number, the farther away from Kevin Bacon the actor is.
For example, Kevin Bacon's Bacon number is 0. If an actor works in a movie with Kevin Bacon, the actor's Bacon number is 1. If an actor works with an actor who worked with Kevin Bacon in a movie, the first actor's Bacon number is 2, and so forth.

Given is the dataset from IMDb in a text file containing two columns ActorID, MovieID.

The task is to find the degrees of separation between any two actors.

Number of actors:	  1,971,697

Number of movies:    12,454,306

Number of records:   17,316,773

File size:             250.4 MB

Benchmark:
25 random queries completed in 13 seconds on 1.6 GHz Intel 8th-gen dual-core i5 processor

Solution:
The algorithm used is Bidirectional Breadth First Search as it converges a lot faster than simple BFS.
Threading is used to leverage multiple cores. This not only executes multiple queries in parallel but also
parallelizes the task of searching for a matching partner when queue size exceeds 2000 entries per iteration in BFS.
This solves the straggler problem when two actors are not related and the whole tree needs to be searched.
To further optimize the runtime, memory mapping technique is used to parse the file into vector maps.
