import sys

if __name__=="__main__":
    raw_data = open(sys.argv[1])
    data = []
    for d in raw_data:
        data.append(d)
    for i in range(1, len(data)):
        print data[i].split(",")[0].strip()