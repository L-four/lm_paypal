// $Id$
function lm_paypal_setbiz(form,user,host)
{
  form.business.value = user + '@' + host;
  return true;
}
