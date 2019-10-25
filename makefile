CXX = g++
CXXFLAGS = -pthread -O2 -std=c++14
LDFLAGS  = -pthread -lboost_program_options

SRCDIR = src
OBJDIR = obj

all: directories server client

.PHONY : all clean

directories:
	@mkdir -p $(OBJDIR)

server: $(OBJDIR)/server.o $(OBJDIR)/logger.o
	$(CXX) -o $@ $^ $(LDFLAGS)

client: $(OBJDIR)/client.o $(OBJDIR)/logger.o
	$(CXX) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS)

clean:
	rm -rf $(OBJDIR) server client