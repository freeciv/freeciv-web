# Fail because a client wasn't found, if it was requested

# FC_NO_CLIENT($which_client, $error_message)
# The $1 client has failed its configure check; it cannot be compiled.  This
# simple macro will exit if this was the requested client, giving an error
# message including $2.  If this wasn't the specified client, it will do
# nothing and configure should continue...

AC_DEFUN([FC_NO_CLIENT],
[
  UL_GUI=$(printf $1 | $SED 's/\./_/g')

  if test "x`eval echo '$'gui_$UL_GUI`" = "xyes"; then
    AC_MSG_ERROR([specified client '$1' not configurable ($2)])
  fi
])
