/* Update hidden types field to reflect checkboxes */
function typesChanged() {
  var typesfield = document.getElementById("types")

  var tt = ""
  if (document.getElementById("cck").checked) tt += "+" // credit
  if (document.getElementById("dck").checked) tt += "-" // debit
  if (document.getElementById("rck").checked) tt += "=" // reset
  if (document.getElementById("lck").checked) tt += "$" // limit
  if (document.getElementById("eck").checked) tt += "!" // error
  typesfield.value = tt;
}

/* Set the type checkboxes from the hidden types field */
function setCheckboxesFromTypesField() {
  var s = document.getElementById("types").value
  document.getElementById("cck").checked = (s.indexOf("+") >= 0)
  document.getElementById("dck").checked = (s.indexOf("-") >= 0)
  document.getElementById("rck").checked = (s.indexOf("=") >= 0)
  document.getElementById("lck").checked = (s.indexOf("$") >= 0)
  document.getElementById("eck").checked = (s.indexOf("!") >= 0)
}
