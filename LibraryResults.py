import sys

if __name__ == "__main__":
  assert((len(sys.argv) - 2) % 3 == 0) #requires triples of files, kloc, runtimes
  to_write_library_details = open(sys.argv[1], 'w')
  library_details = ["name,kloc,number-of-functions,number-of-all-args,number-of-symbolic-args,number-of-fixed-args,number-of-sentinel-args,analysis-time"]
  i = 2
  while i+2 < len(sys.argv):
    results_file = open(sys.argv[i])
    kloc = sys.argv[i+1]
    runtime = sys.argv[i+2]
    library_results = results_file.readline()
    library_results = library_results[:library_results.index(",")+1] + str(kloc) + "," + library_results[library_results.index(",") + 1 :] + "," + str(runtime)
    library_details.append(library_results)
    
    i += 3
  for line in library_details:
    to_write_library_details.write(line + "\n")
