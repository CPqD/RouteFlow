# Copyright 2008 (C) Nicira, Inc.
#
# This file is part of NOX.
#
# NOX is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# NOX is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with NOX.  If not, see <http://www.gnu.org/licenses/>.
#


"""\
Base classes for all factories that produce directories.
Also see nox/lib/directory.py 
"""

class Directory_Factory:
    
    # Authentication types
    AUTH_SIMPLE        = "simple_auth"
    AUTHORIZED_CERT_FP = "authorized_cert_fingerprint"

    # Type support status codes
    NO_SUPPORT         = 'NO'
    READ_ONLY_SUPPORT  = 'RO'
    READ_WRITE_SUPPORT = 'RW'

    # Principal types
    SWITCH_PRINCIPAL       = 0
    LOCATION_PRINCIPAL     = 1
    HOST_PRINCIPAL         = 2
    USER_PRINCIPAL         = 3
    HOST_NETNAME_PRINCIPAL = 4

    ALL_PRINCIPAL_TYPES = (
            SWITCH_PRINCIPAL, LOCATION_PRINCIPAL,
            HOST_PRINCIPAL, USER_PRINCIPAL, HOST_NETNAME_PRINCIPAL
    )

    PRINCIPAL_TYPE_TO_NAME = {
        SWITCH_PRINCIPAL       : "switch principal",
        LOCATION_PRINCIPAL     : "location principal",
        HOST_PRINCIPAL         : "host principal",
        USER_PRINCIPAL         : "user principal",
        HOST_NETNAME_PRINCIPAL : "host netname principal",
    }

    # Group types
    SWITCH_PRINCIPAL_GROUP   = 0
    LOCATION_PRINCIPAL_GROUP = 1
    HOST_PRINCIPAL_GROUP     = 2
    USER_PRINCIPAL_GROUP     = 3
    DLADDR_GROUP             = 4
    NWADDR_GROUP             = 5
    HOST_NETNAME_GROUP       = 6

    ALL_GROUP_TYPES = (
            SWITCH_PRINCIPAL_GROUP, LOCATION_PRINCIPAL_GROUP,
            HOST_PRINCIPAL_GROUP, USER_PRINCIPAL_GROUP,
            DLADDR_GROUP, NWADDR_GROUP, HOST_NETNAME_GROUP,
    )

    GROUP_TYPE_TO_NAME = {
        SWITCH_PRINCIPAL_GROUP   : "switch principal group",
        LOCATION_PRINCIPAL_GROUP : "location principal group",
        HOST_PRINCIPAL_GROUP     : "host principal group",
        USER_PRINCIPAL_GROUP     : "user principal group",
        DLADDR_GROUP             : "datalink address group",
        NWADDR_GROUP             : "network address group",
        HOST_NETNAME_GROUP       : "network binding group",
    }

    PRINCIPAL_GROUP_TO_PRINCIPAL = {
        SWITCH_PRINCIPAL_GROUP   : SWITCH_PRINCIPAL,
        LOCATION_PRINCIPAL_GROUP : LOCATION_PRINCIPAL,
        HOST_PRINCIPAL_GROUP     : HOST_PRINCIPAL,
        USER_PRINCIPAL_GROUP     : USER_PRINCIPAL,
        HOST_NETNAME_GROUP       : HOST_NETNAME_PRINCIPAL,
    }
        
    PRINCIPAL_TO_PRINCIPAL_GROUP = {
        SWITCH_PRINCIPAL       : SWITCH_PRINCIPAL_GROUP,
        LOCATION_PRINCIPAL     : LOCATION_PRINCIPAL_GROUP,
        HOST_PRINCIPAL         : HOST_PRINCIPAL_GROUP,
        USER_PRINCIPAL         : USER_PRINCIPAL_GROUP,
        HOST_NETNAME_PRINCIPAL : HOST_NETNAME_GROUP,
    }
        
    def get_type(self): 
      """Return the name of this type"""
      raise NotImplementedError("get_type must be implemented in "\
                                  "subclass")

    def get_default_config(self):
        """Return string->string dict containing default configuration 
        options to use when configuring new instances of this directory type"""
        return {}

    def get_instance(self, name, config_id):
        """Return deferred returning directory instance for config_id
        """
        raise NotImplementedError("get_instance must be implemented in "\
                                  "subclass")

    def supports_multiple_instances(self):
        """Return bool indicating whether user may create directory instances
        """
        return True
    
    def supports_global_groups(self):
        """Return true iff directory supports groups containing members \
        from other directories.

        If true, all group member names must be mangled with directory
        names.  Should be false in all but special cases.

        When true, all group member names passed in and out of *group*()
        methods should be mangled (including those in GroupInfo instances.)
        """
        return False

    def supported_auth_types(self):
        """Returns tuple of authentication types supported by the directory

        Possible values:
            AUTH_SIMPLE
            AUTHORIZED_CERT_FP
        """
        return ()
    
    def topology_properties_supported(self):
        """Returns one of NO_SUPPORT, READ_ONLY_SUPPORT, or READ_WRITE_SUPPORT
           depending on the capabilities of the directory.
        """
        return Directory_Factory.NO_SUPPORT

    def principal_supported(self, principal_type):
        """Returns one of NO_SUPPORT, READ_ONLY_SUPPORT, or READ_WRITE_SUPPORT
           depending on the capabilities of the directory.
        """
        if principal_type == self.SWITCH_PRINCIPAL:
            return self.switches_supported()
        elif principal_type == self.LOCATION_PRINCIPAL:
            return self.locations_supported()
        elif principal_type == self.HOST_PRINCIPAL:
            return self.hosts_supported()
        elif principal_type == self.USER_PRINCIPAL:
            return self.users_supported()
        elif principal_type == self.HOST_NETNAME_PRINCIPAL:
            return self.host_netnames_supported()
        else:
            return Directory.NO_SUPPORT

    def group_supported(self, group_type):
        """Returns one of NO_SUPPORT, READ_ONLY_SUPPORT, or READ_WRITE_SUPPORT
           depending on the capabilities of the directory.

           This method is ignored and the associated principal groups are not
           supported if principal_supported(group_type) returns NO_SUPPORT
        """
        if group_type == self.SWITCH_PRINCIPAL_GROUP:
            return self.switch_groups_supported()
        elif group_type == self.LOCATION_PRINCIPAL_GROUP:
            return self.location_groups_supported()
        elif group_type == self.HOST_PRINCIPAL_GROUP:
            return self.host_groups_supported()
        elif group_type == self.USER_PRINCIPAL_GROUP:
            return self.user_groups_supported()
        elif group_type == self.DLADDR_GROUP:
            return self.dladdr_groups_supported()
        elif group_type == self.NWADDR_GROUP:
            return self.nwaddr_groups_supported()
        elif group_type == self.HOST_NETNAME_GROUP:
            return self.host_netname_groups_supported()
        else:
            return Directory.NO_SUPPORT

    def switches_supported(self):
        """Returns one of NO_SUPPORT, READ_ONLY_SUPPORT, or READ_WRITE_SUPPORT
           depending on the capabilities of the directory.
        """
        return Directory_Factory.NO_SUPPORT

    def switch_groups_supported(self):
        """Returns one of NO_SUPPORT, READ_ONLY_SUPPORT, or READ_WRITE_SUPPORT
           depending on the capabilities of the directory.

           This method is is ignored and location groups are not supported
           if switches_supported() returns NO_SUPPORT
        """
        return Directory_Factory.NO_SUPPORT

    def locations_supported(self):
        """Returns one of NO_SUPPORT, READ_ONLY_SUPPORT, or READ_WRITE_SUPPORT
           depending on the capabilities of the directory.
        """
        return Directory_Factory.NO_SUPPORT

    def location_groups_supported(self):
        """Returns one of NO_SUPPORT, READ_ONLY_SUPPORT, or READ_WRITE_SUPPORT
           depending on the capabilities of the directory.

           This method is is ignored and location groups are not supported
           if locations_supported() returns NO_SUPPORT
        """
        return Directory_Factory.NO_SUPPORT

    def hosts_supported(self):
        """Returns one of NO_SUPPORT, READ_ONLY_SUPPORT, or READ_WRITE_SUPPORT
           depending on the capabilities of the directory.
        """
        return Directory_Factory.NO_SUPPORT

    def host_groups_supported(self):
        """Returns one of NO_SUPPORT, READ_ONLY_SUPPORT, or READ_WRITE_SUPPORT
           depending on the capabilities of the directory.
        """
        return Directory_Factory.NO_SUPPORT

    def users_supported(self):
        """Returns one of NO_SUPPORT, READ_ONLY_SUPPORT, or READ_WRITE_SUPPORT
           depending on the capabilities of the directory.
        """
        return Directory_Factory.NO_SUPPORT

    def user_groups_supported(self):
        """Returns one of NO_SUPPORT, READ_ONLY_SUPPORT, or READ_WRITE_SUPPORT
           depending on the capabilities of the directory.
        """
        return Directory_Factory.NO_SUPPORT

    def dladdr_groups_supported(self):
        """Returns one of NO_SUPPORT, READ_ONLY_SUPPORT, or READ_WRITE_SUPPORT
           depending on the capabilities of the directory.
        """
        return Directory_Factory.NO_SUPPORT

    def nwaddr_groups_supported(self):
        """Returns one of NO_SUPPORT, READ_ONLY_SUPPORT, or READ_WRITE_SUPPORT
           depending on the capabilities of the directory.
        """
        return Directory_Factory.NO_SUPPORT

    def host_netnames_supported(self):
        """Returns one of NO_SUPPORT, READ_ONLY_SUPPORT, or READ_WRITE_SUPPORT
           depending on the capabilities of the directory.
        """
        #host_netnames are really just a reflection of hosts
        return self.hosts_supported()

    def host_netname_groups_supported(self):
        """Returns one of NO_SUPPORT, READ_ONLY_SUPPORT, or READ_WRITE_SUPPORT
           depending on the capabilities of the directory.
        """
        return Directory_Factory.NO_SUPPORT


