NetMT
=====

A simple multi-threaded tcp server based on boost.asio

###Usage
Inherit from class netmt::Server, implement the abstract method:
'''
/// handle message request
virtual void handle_request(Connection_ptr conn, char* data, std::size_t data_len) = 0;

/// check message whether complete
/// return 0:not complete, <0:error, >0:message length
virtual int check_complete(char* data, std::size_t data_len) = 0;  
'''

Please see example in test folder.

###Compile
need blade and boost support
