function varargout = recv(obj, varargin)
    [buffLen, options] = obj.normalize_msg_options(varargin{:});
    warning(['buffLen ----  ', num2str(buffLen)]);
    if buffLen <= 0
        [varargout{1:nargout}] = zmq.core.recv(obj.socketPointer, varargin{:});
    else
        [varargout{1:nargout}] = zmq.core.recv(obj.socketPointer, buffLen, options{:});
    end
end