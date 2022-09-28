#  Because pybind11 cannot generate default parameters well, this code is to set
#  them

class Profiler:
    """
    Parameters
    ----------
    
    Attributes
    ----------
    Examples
    --------
    >>> import viveka
    """

    def __init__(self):
        super().__init__()

    def set_rank(self, rank, n_procs):
        """Start profiling
        Parameters
        ----------
       
        Returns
        -------
        self: object
            Returns the instance itself.
        """
        return super().set_rank(rank, n_procs)

    def stop(self):
        """Stop profiling
        Parameters
        ----------
        None
        Returns
        -------
        self: object
            Returns the instance itself.
        """
        return super().stop()