import urllib2
import json

f = open("/project/member/regid.txt",'r')
while True:
	url = 'https://android.googleapis.com/gcm/send'
	apiKey = 'AIzaSyB4kdrOqC2MAit76BKBRCbFdcmnfWLvFvk'
	myKey = "key=" + apiKey
	#regid = 'APA91bG1BBReqNtzUDKEH7aK6dPZT6xOJZXpHOhaX-7DqKC4g4Bulmbh0tl46LBb8PGnAjLRgAWWYXSVqpq1rh42RHVcYI5Kndzpja4PIcWmmUqGgLvIGkaxib-3iL8lyOJ9ZVXjcelMo6RBd9YwRts5beyya40vyw'
	regid = f.readline()
	if not regid: break

# make header
	headers = {'Content-Type': 'application/json', 'Authorization': myKey}

# make json data
	data = {}
	data['registration_ids'] = (regid,)
	data['data'] = {'data':'Motion Detect'}
	json_dump = json.dumps(data)
# print json.dumps(data, indent=4)

	req = urllib2.Request(url, json_dump, headers)
	result = urllib2.urlopen(req).read()
	print json.dumps(result)
