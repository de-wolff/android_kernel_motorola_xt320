#ifndef FIFO_H
#define FIFO_H

//#define DEBUG 1
//! Template FIFO class
template <class T> class fifo {

 protected:
	int	len;  // Length of FIFO
	int	wr_ptr; // Write pointer
	int	rd_ptr; // Read pointer
	int count; // Amount of data in FIFO
	T 	*buf;  // Storage

 public:
	fifo(void) : len(0), wr_ptr(0), rd_ptr(0), count(0), buf(NULL) { ; }
	fifo(int len1, T init_value) : len(len1), wr_ptr(0), rd_ptr(0), count(0) {
		buf = new T[len];
		for (int i= 0; i < len; i++)	buf[i] = init_value;
	}
    	fifo(int len1) : len(len1), wr_ptr(0), rd_ptr(0), count(0) {
		buf = new T[len];
	}
	~fifo(void) {	if (len>0) delete [] buf;	}
	void set_size(long n=2) {
		if (len>0) delete [] buf;
		len=n;
		buf = new T[n];
	}
  	int size(void) const {return len;};
	// directly look at data
	T operator [](int i) const {return buf[i];};
	void put(T data_in) {
		if(len>0) {
			buf[wr_ptr++] = data_in;
			wr_ptr = wr_ptr%len;
			count++;
#ifdef DEBUG
			std::cout << "Putting data in fifo, " << this
					  << ", data = "  << data_in
					  << " write pointer = " << wr_ptr
					  << " counter = " << count
					  << "\n";
#endif
			if (count > len) {
				std::cout << "BUFFER OVERFLOW!\n";
			}
			std::cout.flush();
		}
	}
	T get_data() {
		if (len>0) {
			T data = buf[rd_ptr++];
			rd_ptr = rd_ptr%len;
			count--;
#ifdef DEBUG
			std::cout << "Getting data from fifo, " << this
					  << ", data = "  << data
					  << " read pointer = " << rd_ptr
					  << " counter = " << count
					  << "\n";
#endif
			if (count < 0)
				std::cout << "BUFFER UNDERFLOW!\n";
			std::cout.flush();
			return data;
		} else return(-1);
	}
	int get_fullness() { return(count); }
	int get_error_status() {
		if ((count < 0) || (count > len)) return(-1);
		else return(0);
	}
};
#endif
