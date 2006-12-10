find -name .cvsignore | while read file; do
  svn propset svn:ignore "`cat "$file"`" "`echo "$file" | sed 's,/[^/]*$,,'`"
done
