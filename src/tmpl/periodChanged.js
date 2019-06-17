/* Update tmin/tmax based on the period choice */
function periodChanged() {
  var tmin = document.getElementById("tmin");
  var tmax = document.getElementById("tmax");
  var period = document.getElementById("period");
  var value = period.options[period.selectedIndex].value;
  if (value == "") {
    tmin.disabled = tmax.disabled = false
  }
  else {
    tmin.value = tmax.value = "";
    tmin.disabled = tmax.disabled = true
  }
}
