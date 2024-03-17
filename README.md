# About

This project is a collection of tools that can be used for stock investing.

### Tools
- **bvb_scraper_tool** - it's a tool that can used for parsing date from BVB website.
  For more details run `./bvb_scraper_tool --help` in project root directory after
  you have built the project.
- **index_investing_tool** - it's a tool that can used for index replication investments.
  For more details run `./index_investing_tool --help` in project root directory after
  you have built the project.

  Before running this tool you have to create `index_investing_tool.conf` file and fill it
  based on `index_investing_tool.conf.temp` file.

### Supported stock exchanges
For now only these stock exchanges are supported:
- BVB - Bucharest Stock Exchange

### Supported brokers
For now only these brokers are supported:
- TradeVille

# Build

After you cloned the project you should run `git submodule update --recursive --init`.

Before building the project please make sure that you have installed the requirements:
`boost`, `curl`, `openssl`, `gtest`, `cmake`.

Now in order to build the project you have to run `cmake .` and `make` from project root
directory.

# Unit tests

In order to run unit tests just run `./set_unit_tests` from project root directory.
