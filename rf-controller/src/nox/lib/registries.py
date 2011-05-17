import types

class ObjectRegistry:
    """Registry for any type of objects named by a string-type name.

    This class is for handling common registries of objects where it is
    desirable to be able to lookup the object by a string name.  It assumes
    it is not desirable to overwrite an existing registered object, and
    raises an exception if that is attempted."""

    @staticmethod
    def _default_check(o):
        return True

    def __init__(self, objcheckfn=None):
        """Create an object registry.

        The objcheckfun argument is an optional function to be called when
        an object is added to ensure it meets the criteria of the regsitry.
        It is called with a single argument that is the object to check.
        If it returns True, the object will be added to the registry, if
        it returns false it will not.  Note that if it returns false the
        attempt to register will appear to be successful as far as the caller
        is concerned.  If the caller should be informed of an issue, the
        check function should raise an appropriate exception, for example
        a TypeError. If now objcheckfn is provided any object is
        considered acceptable."""
        self._objdict = {}
        if objcheckfn == None:
            self._objcheckfn = ObjectRegistry._default_check
        else:
            self._objcheckfn = objcheckfn

    def has_registered(self, name):
        "Return whether an object is registered in the repository under name."
        return name in self._objdict

    def register(self, name, obj):
        "Register obj in the registry under name."
        ntype = type(name)
        if ntype != types.StringType and ntype != types.UnicodeType:
            raise TypeError("The name for an object must be a regular or unicode string.")
        if self.has_registered(name):
            raise KeyError("The name %s is already registered." % name)
        if self._objcheckfn(obj):
            self._objdict[name] = obj

    def get(self, name):
        "Get the object named by name."
        try:
            return self._objdict[name]
        except KeyError, e:
            raise KeyError("The name %s is not yet registered." % name)

    def list(self, sort_key=None, sort_reverse=None):
        "Return a list of the names of registered objects."
        if sort_key == None:
            return self._objdict.keys()
        o = self._objdict.items()
        o.sort(key=lambda i: sort_key(i[1]), reverse=sort_reverse)
        return [ i[0] for i in o ]

    def __len__(self):
        return len(self._objdict)

    def __getitem__(self, name):
        return self.get(name)

    def __setitem__(self, name, obj):
        self.register(self, name, obj)

    # __delitems__() is deliberately NOT defined because removal of
    # objects from the registery is not allowed.  __setitem__() will
    # also only allow adding new objects, not overwritting existing
    # ones, for the same reason.

    def __iter__(self):
        return self._objdict.iterkeys()

    def __contains__(self, name):
        return self.has_registered(name)

class ClassRegistry(ObjectRegistry):
    """Registry for classes named by a string-type name."""

    def _verify_class(self, o):
        if self.cls == None:
            return True
        if self.cls == o:
            return False
        if not issubclass(o, self.cls):
            raise TypeError("Classes registered in this registry must be subclasses of type %s" % self.cls)
        return True

    def __init__(self, cls=None):
        """Create a class registry.

        If the optional class argument is supplied, all classes being
        registered must be a subclass of that class.  A TypeError will
        be raised on attempts to use any other class and attempts to
        register the class itself will be ignored."""
        self.cls = cls
        ObjectRegistry.__init__(self, self._verify_class)

    def register(self, cls):
        """Register a class in the registry.

        The class must have a class variable called 'name' which is the name
        under which it will be registered."""
        ObjectRegistry.register(self, cls.name, cls)

    def create(self, name, *arg, **kwarg):
        """Create an instance of the class named by 'name'"""
        return self.get(name)(*arg, **kwarg)

class InstanceRegistry(ObjectRegistry):
    """Registry for instances of a class named by a string-type name."""

    def _verify_instance(self, o):
        if self.cls == None:
            return True
        if not isinstance(o, self.cls):
            raise TypeError("Classes registered in this registry must be subclasses of type %s" % self.cls)
        return True

    def __init__(self, cls=None):
        """Create a class registry.

        If the optional class argument is supplied, all instances being
        registered must be a subclass of that class.  A TypeError will
        be raised on attempts to use any other class."""
        self.cls = cls
        ObjectRegistry.__init__(self, self._verify_instance)

    def register(self, name, instance):
        """Register a class in the registry.

        The class must have a class variable called 'name' which is the name
        under which it will be registered."""
        ObjectRegistry.register(self, name, instance)
