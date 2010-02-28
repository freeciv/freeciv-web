rm javascript-compressed/webclient.js -rf

kommandoen="java -jar ./javascript-compressed/compiler.jar --create_source_map freeciv-web-source-map "
for f in `ls javascript/*.js`
do
  kommandoen="$kommandoen --js=$f"
  #cat $f >> javascript-compressed/webclient.js
  #echo ";" >> javascript-compressed/webclient.js 
done

kommandoen="$kommandoen --js_output_file=javascript-compressed/webclient.js"
$kommandoen

