void Senator::Run() {}

std::string Senator::IdentifierString() const {
  std::stringstream ss;
  ss << "Senator [" << ssn_ << ']';
  return ss.str();
}
